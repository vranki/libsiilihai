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
#include "credentialsrequest.h"

static const QString stateNames[] = {"Unknown", "Missing Parser", "Idle", "Updating", "Error", "Updating parser" };

UpdateEngine::UpdateEngine(QObject *parent, ForumDatabase *fd) :
    QObject(parent), fdb(fd)
{
    updateAll = false;
    forceUpdate = false;
    fsubscription = 0;
    currentState = UES_ENGINE_NOT_READY;
    requestingCredentials = false;
    updateWhenEngineReady = false;
    updateOnlyThread = false;
    updateCanceled = true;
    threadBeingUpdated = 0;
    groupBeingUpdated = 0;
    authenticationRetries = 0;
}

UpdateEngine::~UpdateEngine() {
    if(subscription())
        subscription()->engineDestroyed();
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
    disconnect(fsubscription, 0, this, 0);
    fsubscription = 0;
    setState(UES_ENGINE_NOT_READY);
}

QNetworkAccessManager * UpdateEngine::networkAccessManager() {
    return &nam;
}
void UpdateEngine::listGroupsFinished(QList<ForumGroup*> &tempGroups, ForumSubscription *updatedSub) {
    if(updateCanceled) return;
    if(!fsubscription) return;
    Q_ASSERT(updatedSub);
    Q_ASSERT(!groupBeingUpdated);
    Q_ASSERT(!threadBeingUpdated);

    bool dbGroupsWasEmpty = fsubscription->isEmpty();
    fsubscription->markToBeUpdated(false);
    if (tempGroups.size() == 0 && fsubscription->size() > 0) {
        networkFailure("Updating group list for " + subscription()->alias()
                       + " failed. \nCheck your network connection.");
        return;
    }
    // Diff the group list
    bool groupsChanged = false;
    foreach(ForumGroup *tempGroup, tempGroups) {
        bool foundInDb = false;
        foreach(ForumGroup *dbGroup, fsubscription->values()) {
            if (dbGroup->id() == tempGroup->id()) {
                foundInDb = true;

                if(!dbGroup->isSubscribed()) { // If not subscribed, just update the group in db
                    tempGroup->setSubscribed(false);
                    dbGroup->copyFrom(tempGroup);
                    dbGroup->commitChanges();
                }
                if((dbGroup->isSubscribed() &&
                    ((dbGroup->lastchange() != tempGroup->lastchange()) || forceUpdate ||
                     dbGroup->isEmpty() || dbGroup->needsToBeUpdated()))) {
                    dbGroup->markToBeUpdated();
                    qDebug() << Q_FUNC_INFO << "Group " << dbGroup->toString() << " shall be updated";
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
            // qDebug() << "Group " << grp->toString() << " not found in db - adding.";
            groupsChanged = true;
            ForumGroup *newGroup = new ForumGroup(fsubscription, false);
            newGroup->copyFrom(tempGroup);
            newGroup->setChangeset(rand());
            // DON'T set lastchange when only updating group list.
            /* Umm, new group is never subscribed & shouldn't be updated
            if(updateAll && newGroup->isSubscribed()) {
                newGroup->markToBeUpdated();
            }
            */
            fsubscription->addGroup(newGroup);
        }
    }

    // check for DELETED groups
    foreach(ForumGroup *dbGroup, fsubscription->values()) {
        bool groupFound = false;
        foreach(ForumGroup *grp, tempGroups) {
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

    if (updateAll) {
        updateNextChangedGroup();
    } else {
        setState(UES_IDLE);
    }
    if (groupsChanged && dbGroupsWasEmpty) {
        emit(groupListChanged(fsubscription));
    }
}


void UpdateEngine::listThreadsFinished(QList<ForumThread*> &tempThreads, ForumGroup *group) {
    Q_ASSERT(group);
    // Q_ASSERT(!group->isTemp()); temp groups used with parsermaker
    Q_ASSERT(group->isSane());
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
    foreach(ForumThread *serverThread, tempThreads) {
        ForumThread *dbThread = fdb ? fdb->getThread(group->subscription()->id(), group->id(), serverThread->id()) : 0;
        if (dbThread) {
            dbThread->setName(serverThread->name());
            if ((dbThread->lastchange() != serverThread->lastchange()) || forceUpdate ||
                    dbThread->isEmpty() || dbThread->needsToBeUpdated()) {
                // Don't update some fields to new values
                int oldGetMessagesCount = dbThread->getMessagesCount();
                bool oldHasMoreMessages =  dbThread->hasMoreMessages();
                dbThread->setLastchange(serverThread->lastchange());
                dbThread->setOrdernum(serverThread->ordernum());
                dbThread->setChangeset(serverThread->changeset());
                dbThread->setGetMessagesCount(oldGetMessagesCount);
                dbThread->setHasMoreMessages(oldHasMoreMessages);
                dbThread->commitChanges();
                dbThread->markToBeUpdated();
                threadsToUpdateQueue.enqueue(dbThread);
            }
        } else {
            ForumThread *newThread = new ForumThread(group, false);
            newThread->copyFrom(serverThread);
            newThread->setChangeset(-1);
            group->addThread(newThread, false);
            newThread->markToBeUpdated();
            threadsToUpdateQueue.enqueue(newThread);
        }
    }
    QSet<ForumThread*> deletedThreads;
    // check for DELETED threads
    foreach (ForumThread *dbThread, group->values()) { // Iterate all db threads and find if any is missing
        bool threadFound = false;
        foreach(ForumThread *tempThread, tempThreads) {
            if (dbThread->group()->id() == group->id() && dbThread->id() == tempThread->id()) {
                threadFound = true;
            }
        }
        if (!threadFound) {
            deletedThreads.insert(dbThread);
        }
    }
    foreach(ForumThread *thr, deletedThreads.values())
        thr->group()->removeThread(thr);

    group->markToBeUpdated(false);
    threadBeingUpdated = 0;

    if(updateAll)
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
    bool messagesChanged = false; // @todo this is not used anywhere.. should be deleted?
    foreach (ForumMessage *tempMessage, tempMessages) {
        bool foundInDb = false;
        foreach (ForumMessage *dbMessage, dbThread->values()) {
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
            messagesChanged = true;
            ForumMessage *newMessage = new ForumMessage(dbThread, false);
            newMessage->copyFrom(tempMessage);
            newMessage->markToBeUpdated(false);
            dbThread->addMessage(newMessage);
        }
    }

    // check for DELETED threads
    foreach (ForumMessage *dbmessage, dbThread->values()) {
        bool messageFound = false;
        foreach (ForumMessage *tempMsg, tempMessages) {
            if (dbmessage->id() == tempMsg->id()) {
                messageFound = true;
            }
        }
        if (!messageFound) { // @todo don't delete, if tempMessages doesn't start from first page!!
            // @todo are ordernums ok then? This is probably causing a bug.
            messagesChanged = true;
            dbThread->removeMessage(dbmessage);
        }
    }
    // update thread
    dbThread->setHasMoreMessages(moreAvailable);
    dbThread->commitChanges();
    threadBeingUpdated = 0;
    if(updateAll) {
        updateNextChangedThread();
    } else {
        groupBeingUpdated = 0;
        setState(UES_IDLE);
        emit forumUpdated(subscription());
    }
}

void UpdateEngine::networkFailure(QString message) {
    if(!updateCanceled)
        emit updateFailure(fsubscription, "Updating " + fsubscription->alias() + " failed due to network error:\n\n" + message);
    setState(UES_ERROR);
}

void UpdateEngine::loginFinishedSlot(ForumSubscription *sub, bool success) {
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
    Q_ASSERT(updateAll);
    Q_ASSERT(!subscription()->beingSynced());
    Q_ASSERT(!updateCanceled);
    Q_ASSERT(!threadBeingUpdated);
    Q_ASSERT(!groupBeingUpdated);
    Q_ASSERT(state() == UES_UPDATING);

    if(!updateOnlyThread) {
        foreach(ForumGroup *group, subscription()->values()) {
            if(group->needsToBeUpdated() && group->isSubscribed()) {
                doUpdateGroup(group);
                return;
            }
        }
    }
    //  qDebug() << Q_FUNC_INFO << "No more changed groups - end of update.";
    updateAll = false;
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
        threadBeingUpdated = 0;
        groupBeingUpdated = 0;
        updateNextChangedGroup();
    }
    updateCurrentProgress();
}

void UpdateEngine::cancelOperation() {
    updateCanceled = true;
    if(currentState==UES_IDLE || currentState==UES_ERROR) return;
    updateAll = false;
    foreach(ForumGroup *group, subscription()->values())
        group->markToBeUpdated(false);
    threadsToUpdateQueue.clear();
    if(currentState==UES_UPDATING)
        setState(UES_IDLE);
    if(requestingCredentials)
        emit loginFinished(subscription(), false);
    requestingCredentials = false;
    threadBeingUpdated = 0;
    groupBeingUpdated = 0;
    authenticationRetries = 0;
}

void UpdateEngine::updateCurrentProgress() {
    float progress = -1;
    emit statusChanged(fsubscription, progress);
}

void UpdateEngine::setState(UpdateEngineState newState) {
    if(newState == currentState) return;
    UpdateEngineState oldState = currentState;
    currentState = newState;

    // Caution: subscription may be null!
    if(subscription())
        qDebug() << Q_FUNC_INFO << subscription()->alias() << stateNames[oldState] << " -> " << stateNames[newState];
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
        cancelOperation();
        emit forumUpdated(subscription());
    }
    emit stateChanged(currentState, oldState);
}

void UpdateEngine::requestCredentials() {
    requestingCredentials = true;
}

void UpdateEngine::credentialsEntered(CredentialsRequest* cr) {
    requestingCredentials = false;
    if(cr->credentialType == CredentialsRequest::SH_CREDENTIAL_FORUM) {
        if(cr->authenticator.user().length() > 0) {
            subscription()->setUsername(cr->authenticator.user());
            subscription()->setPassword(cr->authenticator.password());
            subscription()->setAuthenticated(true);
            authenticationRetries++;
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
    return false;
}

void UpdateEngine::updateForum(bool force) {
    qDebug() << Q_FUNC_INFO << " force: " << force << subscription()->toString();
    Q_ASSERT(fsubscription);
    Q_ASSERT(!fsubscription->beingSynced());
    Q_ASSERT(!fsubscription->scheduledForSync());
    Q_ASSERT(!groupBeingUpdated);
    Q_ASSERT(!threadBeingUpdated);

    forceUpdate = force;
    updateOnlyThread = false;
    updateCanceled = false;
    updateAll = true; // Update whole forum
    authenticationRetries = 0;
    if(state()==UES_ENGINE_NOT_READY) {
        updateWhenEngineReady = true;
    } else {
        setState(UES_UPDATING);
    }
}


void UpdateEngine::updateThread(ForumThread *thread, bool force) {
    Q_ASSERT(fsubscription);
    Q_ASSERT(thread);
    Q_ASSERT(!thread->group()->subscription()->beingSynced());
    Q_ASSERT(!thread->group()->subscription()->scheduledForSync());
    Q_ASSERT(!groupBeingUpdated);
    Q_ASSERT(!threadBeingUpdated);
    Q_ASSERT(currentState==UES_IDLE);

    forceUpdate = force;
    updateAll = true;
    updateOnlyThread = true;
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

void UpdateEngine::updateGroupList() {
    updateAll = false; // Just the group list
    updateOnlyThread = false;
    updateCanceled = false;
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
    } else {
        Q_ASSERT(!groupBeingUpdated);
        Q_ASSERT(!threadBeingUpdated);
        Q_ASSERT(state() == UES_UPDATING);
        doUpdateForum();
    }
}
