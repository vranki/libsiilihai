/* This file is part of libSiilihai.

    libSiilihai is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libSiilihai is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with libSiilihai.  If not, see <http://www.gnu.org/licenses/>. */
#include "updateengine.h"
#include "forumdata/forumgroup.h"
#include "forumdata/forumthread.h"
#include "forumdata/forummessage.h"
#include "forumdatabase/forumdatabase.h"
#include "forumdata/forumsubscription.h"
#include "forumdata/updateerror.h"
#include "credentialsrequest.h"
#include "parser/forumsubscriptionparsed.h"
#include "parser/parserengine.h"
#include "parser/parsermanager.h"
#include "tapatalk/tapatalkengine.h"
#include "discourse/discourseengine.h"

static const QString stateNames[] = {"Unknown", "Missing Parser", "Idle", "Updating", "Error", "Updating parser" };

UpdateEngine::UpdateEngine(QObject *parent, ForumDatabase *fd) :
    QObject(parent)
  , forceUpdate(false)
  , requestingCredentials(false)
  , currentState(UES_ENGINE_NOT_READY)
  , updateWhenEngineReady(false)
  , updateOnlyThread(false)
  , updateOnlyGroup(false)
  , updateOnlyGroups(false)
  , updateCanceled(true)
  , threadBeingUpdated(nullptr)
  , groupBeingUpdated(nullptr)
  , fsubscription(nullptr)
  , fdb(fd)
  , authenticationRetries(0)
  , lastProgressReport(-1)
{ }

UpdateEngine::~UpdateEngine() {
    if(subscription()) subscription()->engineDestroyed();
}

UpdateEngine *UpdateEngine::newForSubscription(ForumSubscription *fs,
                                               ForumDatabase *fdb,
                                               ParserManager *pm)
{
    Q_ASSERT(pm);
    UpdateEngine *ue = nullptr;

    if(fs->provider() == ForumSubscription::FP_PARSER) {
        ForumSubscriptionParsed *newFsParsed = qobject_cast<ForumSubscriptionParsed*>(fs);
        ParserEngine *pe = new ParserEngine(fdb, fdb, pm);
        ue = pe;
        ue->setSubscription(newFsParsed);
        pe->setParser(pm->getParser(newFsParsed->parserId()));
        // wat?
        // if(!pe->parser()) pe->setParser(pm->getParser(newFsParsed->parserId())); // Load the (possibly old) parser
    } else if(fs->provider() == ForumSubscription::FP_TAPATALK) {
        TapaTalkEngine *tte = new TapaTalkEngine(fdb, fdb);
        ue = tte;
    } else if(fs->provider() == ForumSubscription::FP_DISCOURSE) {
        DiscourseEngine *de = new DiscourseEngine(fdb, fdb);
        ue = de;
    }
    return ue;
}

void UpdateEngine::setSubscription(ForumSubscription *fs) {
    ForumSubscription *oldSub = fsubscription;
    fsubscription = fs;
    if(fsubscription) {
        connect(fsubscription, SIGNAL(destroyed()), this, SLOT(subscriptionDeleted()));
    } else {
        if(oldSub) {
            disconnect(oldSub, 0, this, 0);
        }
        setState(UES_ENGINE_NOT_READY);
    }
}

UpdateEngine::UpdateEngineState UpdateEngine::state() {
    return currentState;
}

ForumSubscription* UpdateEngine::subscription() const {
    return fsubscription;
}

void UpdateEngine::subscriptionDeleted() {
    //cancelOperation(); causes trouble
    disconnect(fsubscription, nullptr, this, nullptr);
    fsubscription = nullptr;
    resetState();
    setState(UES_ENGINE_NOT_READY);
}

QNetworkAccessManager* UpdateEngine::networkAccessManager() {
    return &nam;
}

void UpdateEngine::resetState() {
    updateCanceled = true;
    updateOnlyGroups = false;
    updateOnlyGroup = false;
    updateOnlyThread = false;
    forceUpdate = false;
    updateWhenEngineReady = false;

    if(subscription()) {
        for(ForumGroup *group : *subscription())
            group->markToBeUpdated(false);
    }
    threadsToUpdateQueue.clear();
    if(currentState==UES_UPDATING) setState(UES_IDLE);
    if(requestingCredentials) emit loginFinished(subscription(), false);
    requestingCredentials = false;
    threadBeingUpdated = nullptr;
    groupBeingUpdated = nullptr;
    authenticationRetries = 0;
    lastProgressReport = -1;
}

void UpdateEngine::listGroupsFinished(QList<ForumGroup*> &tempGroups, ForumSubscription *updatedSub) {
    if(updateCanceled) return;
    if(!fsubscription) return;
    Q_ASSERT(updatedSub);
    Q_ASSERT(!groupBeingUpdated);
    Q_ASSERT(!threadBeingUpdated);

    bool dbGroupsWasEmpty = fsubscription->isEmpty();
    if (tempGroups.isEmpty()) {
        networkFailure("Updating group list for " + subscription()->alias()
                       + " failed. \nCheck your network connection.");
        return;
    }
    // Diff the group list
    bool groupsChanged = false;
    for (ForumGroup *tempGroup : tempGroups) {
        bool foundInDb = false;
        QString tempGroupId = tempGroup->id();
        for(ForumGroup *dbGroup : *fsubscription) {
            if (dbGroup->id() == tempGroupId) {
                foundInDb = true;

                if(!dbGroup->isSubscribed()) { // If not subscribed, just update the group in db
                    tempGroup->setSubscribed(false);
                    dbGroup->copyFrom(tempGroup);
                    dbGroup->commitChanges();
                }
                if((dbGroup->isSubscribed() &&
                    ((dbGroup->lastChange() != tempGroup->lastChange()) || forceUpdate ||
                     dbGroup->isEmpty() || dbGroup->needsToBeUpdated()))) {
                    dbGroup->markToBeUpdated();
                    // Store the updated version to database
                    tempGroup->setSubscribed(true);
                    dbGroup->copyFrom(tempGroup);
                    dbGroup->commitChanges();
                } else {
                    //         qDebug() << "Group" << dbgroup->toString()
                    //                << " hasn't changed or is not subscribed - not reloading.";
                }
            }
        }
        if (!foundInDb) {
            qDebug() << "Group " << tempGroup->toString() << " not found in db - adding.";
            groupsChanged = true;
            ForumGroup *newGroup = new ForumGroup(fsubscription, fsubscription->isTemp());
            qDebug() << Q_FUNC_INFO << newGroup->isTemp() << fsubscription->isTemp();
            newGroup->copyFrom(tempGroup);
            newGroup->setChangeset(rand());
            // DON'T set lastchange when only updating group list.
            /* Umm, new group is never subscribed & shouldn't be updated
            if(updateAll && newGroup->isSubscribed()) {
                newGroup->markToBeUpdated();
            }
            */
            qDebug() << Q_FUNC_INFO << newGroup->isTemp() << fsubscription->isTemp();
            fsubscription->addGroup(newGroup);
        }
    }

    // check for DELETED groups
    for(ForumGroup *dbGroup : *fsubscription) {
        bool groupFound = false;
        for(ForumGroup *grp : tempGroups) {
            if (dbGroup->id() == grp->id()) {
                groupFound = true;
            }
        }
        if (!groupFound) {
            groupsChanged = true;
            qDebug() << Q_FUNC_INFO << "Group " << dbGroup->toString() << " has been deleted!";
            fsubscription->removeGroup(dbGroup);
        }
    }

    if (!updateOnlyGroups) {
        updateNextChangedGroup();
    } else {
        fsubscription->markToBeUpdated(false);
        setState(UES_IDLE);
    }
    if (groupsChanged && dbGroupsWasEmpty) {
        emit(groupListChanged(fsubscription));
    }
}

void UpdateEngine::listThreadsFinished(QList<ForumThread*> &tempThreads, ForumGroup *group) {
    Q_ASSERT(group);
    // Q_ASSERT(group->isSane()); can be insane in parsermaker
    Q_ASSERT(!group->subscription()->beingSynced());
    Q_ASSERT(!groupBeingUpdated);
    Q_ASSERT(!threadBeingUpdated);
    if(updateCanceled) return;
    threadsToUpdateQueue.clear();

    if(!group->isSubscribed()) { // Unsubscribed while update
        qDebug() << Q_FUNC_INFO << "Group" << group->toString() << " not subscribed! Ignoring these.";
        return;
    }

    if (tempThreads.isEmpty() && !group->isEmpty()) {
        QString errorMsg = "Found no threads in group " + group->toString() + ".\n Broken parser?";
        group->markToBeUpdated();
        networkFailure(errorMsg);
        return;
    }

    // Diff the group list
    for(ForumThread *serverThread : tempThreads) {
        ForumThread *dbThread = fdb ? fdb->getThread(group->subscription()->id(), group->id(), serverThread->id()) : 0;
        if (dbThread) {
            dbThread->setName(serverThread->name());
            if ((dbThread->lastChange() != serverThread->lastChange()) || forceUpdate ||
                    dbThread->isEmpty() || dbThread->needsToBeUpdated()) {
                // Don't update some fields to new values
                int oldGetMessagesCount = dbThread->getMessagesCount();
                bool oldHasMoreMessages =  dbThread->hasMoreMessages();
                dbThread->setLastChange(serverThread->lastChange());
                dbThread->setOrdernum(serverThread->ordernum());
                dbThread->setChangeset(serverThread->changeset());
                dbThread->setGetMessagesCount(oldGetMessagesCount);
                dbThread->setHasMoreMessages(oldHasMoreMessages);
                dbThread->commitChanges();
                dbThread->markToBeUpdated();
                threadsToUpdateQueue.enqueue(dbThread);
            }
        } else {
            if(group->contains(serverThread->id())) {
                qDebug() << Q_FUNC_INFO
                         << "Warning: group "
                         << group->toString()
                         << "already contains thread with id"
                         << serverThread->id()
                         << "not adding it. Broken parser?";
            } else {
                ForumThread *newThread = new ForumThread(group, false);
                newThread->copyFrom(serverThread);
                newThread->setChangeset(-1);
                group->addThread(newThread, false);
                newThread->markToBeUpdated();
                threadsToUpdateQueue.enqueue(newThread);
            }
        }
    }
    QSet<ForumThread*> deletedThreads;
    // check for DELETED threads
    for (ForumThread *dbThread : *group) { // Iterate all db threads and find if any is missing
        bool threadFound = false;
        for(ForumThread *tempThread : tempThreads) {
            if (dbThread->group()->id() == group->id() && dbThread->id() == tempThread->id()) {
                threadFound = true;
            }
        }
        if (!threadFound) {
            deletedThreads.insert(dbThread);
        }
    }
    for(ForumThread *thr : deletedThreads.values())
        thr->group()->removeThread(thr);

    group->markToBeUpdated(false);
    threadBeingUpdated = nullptr;

    group->threadsChanged();

    if(!updateOnlyGroup)
        updateNextChangedThread();
}


void UpdateEngine::listMessagesFinished(QList<ForumMessage*> &tempMessages, ForumThread *dbThread, bool moreAvailable) {
    Q_ASSERT(dbThread);
    Q_ASSERT(dbThread->isSane());
    // Q_ASSERT(!dbThread->isTemp()); temp threads used with parsermaker
    Q_ASSERT(dbThread->group()->isSubscribed());
    // Q_ASSERT(groupBeingUpdated); can be null or set
    Q_ASSERT(!threadBeingUpdated);
    if(updateCanceled) return;

    dbThread->markToBeUpdated(false);
    if(tempMessages.size()==0) qDebug() << Q_FUNC_INFO << "got 0 messages in thread " << dbThread->toString();

    for (ForumMessage *tempMessage : tempMessages) {
        bool foundInDb = false;
        for (ForumMessage *dbMessage : *dbThread) {
            if (dbMessage->id() == tempMessage->id()) {
                foundInDb = true;
                bool wasRead = dbMessage->isRead();
                dbMessage->copyFrom(tempMessage);
                if(wasRead) dbMessage->setRead(true, false);
                dbMessage->markToBeUpdated(false);
                dbMessage->commitChanges();
            }
        }
        if (!foundInDb) {
            ForumMessage *newMessage = new ForumMessage(dbThread, false);
            newMessage->copyFrom(tempMessage);
            newMessage->markToBeUpdated(false);
            dbThread->addMessage(newMessage);
        }
    }

    // Find deleted messages
    QList<ForumMessage*> messagesToDelete;
    for(ForumMessage *dbmessage : *dbThread) {
        bool messageFound = false;
        for(ForumMessage *tempMsg : tempMessages)
            if (dbmessage->id() == tempMsg->id())
                messageFound = true;
        if (!messageFound) { // @todo don't delete, if tempMessages doesn't start from first page!!
            // @todo are ordernums ok then? This is probably causing a bug.
            messagesToDelete.append(dbmessage);
            Q_ASSERT(dbmessage->thread() == dbThread);
        }
    }
    // Delete the deleted messages
    for(ForumMessage *msg : messagesToDelete)
        msg->thread()->removeMessage(msg);

    // update thread
    dbThread->setHasMoreMessages(moreAvailable);
    dbThread->commitChanges();
    dbThread->messagesChanged();
    threadBeingUpdated = nullptr;
    if(!updateOnlyThread) {
        updateNextChangedThread();
    } else {
        groupBeingUpdated = nullptr;
        setState(UES_IDLE);
        emit forumUpdated(subscription());
    }
}

void UpdateEngine::networkFailure(const QString &message, const QString &technical) {
    if(!updateCanceled)
        subscription()->appendError(new UpdateError("Network error", message, technical));
    setState(UES_ERROR);
}

void UpdateEngine::loginFinishedSlot(ForumSubscription *sub, bool success) {
    Q_UNUSED(sub);
    if(state()!=UES_UPDATING) {
        qDebug() << Q_FUNC_INFO << "We're not updating so i'll ignore this as a old reply";
        return;
    }
    if(success) {
        continueUpdate();
    } else {
        requestCredentials();
    }
}

void UpdateEngine::updateNextChangedGroup() {
    Q_ASSERT(!subscription()->beingSynced());
    Q_ASSERT(!updateCanceled);
    Q_ASSERT(!threadBeingUpdated);
    Q_ASSERT(state() == UES_UPDATING);

    if(!updateOnlyThread && !updateOnlyGroup) {
        for(ForumGroup *group : *subscription()) {
            if(group->needsToBeUpdated() && group->isSubscribed()) {
                doUpdateGroup(group);
                return;
            }
        }
    }
    //  qDebug() << Q_FUNC_INFO << "No more changed groups - end of update.";
    Q_ASSERT(threadsToUpdateQueue.isEmpty());
    setState(UES_IDLE);
    emit forumUpdated(subscription());
    updateCurrentProgress();
}

void UpdateEngine::updateNextChangedThread() {
    Q_ASSERT(!subscription()->beingSynced());
    Q_ASSERT(!updateCanceled);
    Q_ASSERT(!threadBeingUpdated);
    Q_ASSERT(state() == UES_UPDATING);

    if (!threadsToUpdateQueue.isEmpty()) {
        ForumThread *thread = threadsToUpdateQueue.dequeue();
        if(forceUpdate)
            thread->setLastPage(0);
        doUpdateThread(thread);
    } else {
        threadBeingUpdated = nullptr;
        groupBeingUpdated = nullptr;
        updateNextChangedGroup();
    }
    updateCurrentProgress();
}

void UpdateEngine::cancelOperation() {
    resetState();
    if(currentState==UES_IDLE || currentState==UES_ERROR) return;
}

void UpdateEngine::updateCurrentProgress() {
    float progress = 0;
    if(state() != UES_UPDATING) {
        progress = 1;
    } else {
        unsigned int subscribedGroups = 0;

        for(ForumGroup *grp : *subscription()) {
            if (grp->isSubscribed()) subscribedGroups++;
        }

        if(subscribedGroups) {
            unsigned int groupsUpdated = 0;
            for(ForumGroup *grp : *subscription()) {
                if(!grp->needsToBeUpdated()) groupsUpdated++;
            }
            progress = groupsUpdated / subscribedGroups;
        }
    }

    if(lastProgressReport != progress) {
        lastProgressReport = progress;
        emit progressReport(subscription(), progress);
    }
}

void UpdateEngine::setState(UpdateEngineState newState) {
    if(newState == currentState) return;
    UpdateEngineState oldState = currentState;
    currentState = newState;
    updateCurrentProgress();
    // Caution: subscription may be null!
    if(subscription())
        qDebug() << Q_FUNC_INFO << subscription()->alias() << stateName(oldState) << " -> " << stateName(newState);
    //
    if(newState==UES_UPDATING) {
        Q_ASSERT(oldState==UES_IDLE || oldState==UES_ERROR);
        Q_ASSERT(subscription());
        updateWhenEngineReady = false;
        if(subscription() && subscription()->isAuthenticated()
                && subscription()->username().isEmpty()
                && !requestingCredentials) {
            qDebug() << Q_FUNC_INFO << "Forum " << subscription()->alias() << "requires authentication. Asking for it. U:"
                     << subscription()->username();
            requestCredentials();
        }
        subscription()->setScheduledForUpdate(false);
        subscription()->setBeingUpdated(true);
        if(requestingCredentials) {
            qDebug() << Q_FUNC_INFO << "Requesting credentials - NOT updating!";
        } else {
            continueUpdate();
        }
    }
    if(newState==UES_IDLE) {
        subscription()->setBeingUpdated(false);
        if(updateWhenEngineReady) {
            updateForum(forceUpdate);
            return;
        }
    }
    if(newState==UES_ENGINE_NOT_READY) {
        if(subscription())
            cancelOperation();
    }
    if(newState==UES_ERROR) {
        subscription()->setScheduledForUpdate(false);
        subscription()->setBeingUpdated(false);
        emit forumUpdated(subscription());
        cancelOperation();
    }
    emit stateChanged(this, currentState, oldState);
}

void UpdateEngine::requestCredentials() {
    requestingCredentials = true;
}

void UpdateEngine::credentialsEntered(CredentialsRequest* cr) {
    requestingCredentials = false;
    if(cr->credentialType == CredentialsRequest::SH_CREDENTIAL_FORUM) {
        if(cr->username().length() > 0) {
            subscription()->setUsername(cr->username());
            subscription()->setPassword(cr->password());
            subscription()->setAuthenticated(true);
            authenticationRetries++;
            qDebug() << Q_FUNC_INFO << "Auth retry" << authenticationRetries;
            if(authenticationRetries > 3) {
                qDebug() << Q_FUNC_INFO << "Too many credential retries - erroring";
                networkFailure("Authentication failed");
            }
        } else {
            subscription()->setAuthenticated(false);
            subscription()->setGroupListChanged();
        }
    } else { // HTTP creds
    }
    // Start updating when credentials entered
    if(currentState==UES_UPDATING) {
        qDebug() << Q_FUNC_INFO << "resuming update now";
        continueUpdate();
    }
}

bool UpdateEngine::postMessage(ForumGroup *grp, ForumThread *thr, QString subject, QString body) {
    Q_UNUSED(grp);
    Q_UNUSED(thr);
    Q_UNUSED(subject);
    Q_UNUSED(body);
    return false;
}

void UpdateEngine::updateForum(bool force) {
    Q_ASSERT(fsubscription);
    Q_ASSERT(!fsubscription->beingSynced());
    Q_ASSERT(!fsubscription->scheduledForSync());
    Q_ASSERT(!groupBeingUpdated);
    Q_ASSERT(!threadBeingUpdated);
    resetState();
    updateCanceled = false;
    forceUpdate = force;
    subscription()->clearErrors();
    if(state()==UES_ENGINE_NOT_READY) {
        updateWhenEngineReady = true;
        qDebug() << Q_FUNC_INFO << "Engine not ready for" << fsubscription->alias();
    } else {
        qDebug() << Q_FUNC_INFO << "Updating" << fsubscription->alias();
        setState(UES_UPDATING);
    }
}

void UpdateEngine::updateGroup(ForumGroup *group, bool onlyGroup)
{
    Q_ASSERT(group);
    if(group->subscription()->latestThreads() == 0)
        qWarning() << Q_FUNC_INFO << "Warning: subscription << " << group->subscription()->alias() << " latest threads is zero! Nothing can be found!";

    resetState();
    updateCanceled = false;
    updateOnlyGroup = onlyGroup;
    doUpdateGroup(group);
}


void UpdateEngine::updateThread(ForumThread *thread, bool force, bool onlyThread) {
    Q_ASSERT(fsubscription);
    Q_ASSERT(thread);
    Q_ASSERT(!thread->group()->subscription()->beingSynced());
    Q_ASSERT(!thread->group()->subscription()->scheduledForSync());
    Q_ASSERT(!groupBeingUpdated);
    Q_ASSERT(!threadBeingUpdated);
    Q_ASSERT(currentState==UES_IDLE);
    resetState();

    if(thread->group()->subscription()->latestMessages() == 0)
        qWarning() << Q_FUNC_INFO << "Warning: subscription << " << thread->group()->subscription()->alias() << " latest messages is zero! Nothing can be found!";
    updateOnlyThread = onlyThread;
    updateCanceled = false;

    if(force) {
        thread->setLastPage(0);
        thread->markToBeUpdated();
    }
    if(!threadsToUpdateQueue.contains(thread))
        threadsToUpdateQueue.enqueue(thread);

    setState(UES_UPDATING);
}

bool UpdateEngine::supportsPosting() {
    return false;
}

QString UpdateEngine::convertDate(QString &date) {
    return date;
}

QString UpdateEngine::stateName(UpdateEngine::UpdateEngineState state)
{
    return stateNames[state];
}

void UpdateEngine::updateGroupList() {
    resetState();
    updateCanceled = false;
    updateOnlyGroups = true;
    updateWhenEngineReady = true;

    Q_ASSERT(!groupBeingUpdated);
    Q_ASSERT(!threadBeingUpdated);

    if(state() != UES_ENGINE_NOT_READY) {
        setState(UES_UPDATING);
    }
}

void UpdateEngine::continueUpdate() {
    if(updateOnlyThread) {
        updateNextChangedThread();
    } else if (updateOnlyGroup) {
        doUpdateGroup(groupBeingUpdated);
    } else {
        Q_ASSERT(!groupBeingUpdated);
        Q_ASSERT(!threadBeingUpdated);
        Q_ASSERT(state() == UES_UPDATING);
        doUpdateForum();
    }
}
