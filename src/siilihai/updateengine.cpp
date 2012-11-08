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
    currentState = PES_ENGINE_NOT_READY;
    requestingCredentials = false;
    updateWhenEngineReady = false;
    updateOnlyThread = false;
}

UpdateEngine::~UpdateEngine() {
}

void UpdateEngine::setSubscription(ForumSubscription *fs) {
    fsubscription = fs;
    if(fs) {
        connect(fsubscription, SIGNAL(destroyed()), this, SLOT(subscriptionDeleted()));
    } else {
        setState(PES_ENGINE_NOT_READY);
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
    setState(PES_ENGINE_NOT_READY);
}

QNetworkAccessManager * UpdateEngine::networkAccessManager() {
    return &nam;
}

void UpdateEngine::listMessagesFinished(QList<ForumMessage*> &tempMessages, ForumThread *dbThread, bool moreAvailable) {
    Q_ASSERT(dbThread);
    Q_ASSERT(dbThread->isSane());
    Q_ASSERT(!dbThread->isTemp());
    Q_ASSERT(dbThread->group()->isSubscribed());
    dbThread->markToBeUpdated(false);
    if(tempMessages.size()==0) qDebug() << Q_FUNC_INFO << "got 0 messages in thread " << dbThread->toString();
    bool messagesChanged = false;
    foreach (ForumMessage *tempMessage, tempMessages) {
        bool foundInDb = false;
        foreach (ForumMessage *dbMessage, dbThread->values()) {
            if (dbMessage->id() == tempMessage->id()) {
                foundInDb = true;
                Q_ASSERT(tempMessage->thread() == dbMessage->thread());
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
        foreach (ForumMessage *msg, tempMessages) {
            if (dbThread->group()->subscription()->forumId() == msg->thread()->group()->subscription()->forumId() &&
                    dbThread->group()->id() == msg->thread()->group()->id() &&
                    dbThread->id() == msg->thread()->id() &&
                    dbmessage->id() == msg->id()) {
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
    if(updateAll) {
        updateNextChangedThread();
    } else {
        setState(PES_IDLE);
        emit forumUpdated(subscription());
    }
    fdb->checkSanity();
}

void UpdateEngine::listGroupsFinished(QList<ForumGroup*> &tempGroups) {
    if(!fsubscription) return;
    bool dbGroupsWasEmpty = fsubscription->isEmpty();
    fsubscription->markToBeUpdated(false);
    if (tempGroups.size() == 0 && fsubscription->size() > 0) {
        emit updateFailure(fsubscription, "Updating group list for " + subscription()->alias()
                           + " failed. \nCheck your network connection.");
        setState(PES_ERROR);
        return;
    }
    // Diff the group list
    bool groupsChanged = false;
    foreach(ForumGroup *tempGroup, tempGroups) {
        bool foundInDb = false;
        foreach(ForumGroup *dbGroup, fsubscription->values()) {
            if (dbGroup->id() == tempGroup->id()) {
                foundInDb = true;
                if((dbGroup->isSubscribed() &&
                    ((dbGroup->lastchange() != tempGroup->lastchange()) || forceUpdate ||
                     dbGroup->isEmpty() || dbGroup->needsToBeUpdated()))) {
                    dbGroup->markToBeUpdated();
                    qDebug() << Q_FUNC_INFO << "Group " << dbGroup->toString() << " shall be updated";
                    // Store the updated version to database
                    tempGroup->setSubscribed(true);
                    Q_ASSERT(tempGroup->subscription() == dbGroup->subscription());
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
            if(updateAll && newGroup->isSubscribed()) {
                newGroup->markToBeUpdated();
            }
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
        setState(PES_IDLE);
    }
    if (groupsChanged && dbGroupsWasEmpty) {
        emit(groupListChanged(fsubscription));
    }

    fdb->checkSanity();
}


void UpdateEngine::listThreadsFinished(QList<ForumThread*> &tempThreads, ForumGroup *group) {
    Q_ASSERT(group);
    Q_ASSERT(!group->isTemp());
    Q_ASSERT(group->isSane());
    Q_ASSERT(!group->subscription()->beingSynced());
    threadsToUpdateQueue.clear();

    if(!group->isSubscribed()) { // Unsubscribed while update
        qDebug() << Q_FUNC_INFO << "Group" << group->toString() << " not subscribed! Ignoring these.";
        return;
    }

    if (tempThreads.isEmpty() && !group->isEmpty()) {
        QString errorMsg = "Found no threads in group " + group->toString() + ".\n Broken parser?";
        emit updateFailure(subscription(), errorMsg);
        group->markToBeUpdated();
        setState(PES_ERROR);
        return;
    }

    // Diff the group list
    foreach(ForumThread *serverThread, tempThreads) {
        ForumThread *dbThread = fdb->getThread(group->subscription()->forumId(), group->id(), serverThread->id());
        if (dbThread) {
            dbThread->setName(serverThread->name());
            if ((dbThread->lastchange() != serverThread->lastchange()) || forceUpdate ||
                    dbThread->isEmpty() || dbThread->needsToBeUpdated()) {
                Q_ASSERT(dbThread->group() == serverThread->group());

                // Don't update some fields to new values
                int oldGetMessagesCount = dbThread->getMessagesCount();
                bool oldHasMoreMessages =  dbThread->hasMoreMessages();
                dbThread->setLastchange(serverThread->lastchange());
                dbThread->setOrdernum(serverThread->ordernum());
                dbThread->setChangeset(serverThread->changeset());
                dbThread->setGetMessagesCount(oldGetMessagesCount);
                dbThread->setHasMoreMessages(oldHasMoreMessages);
                dbThread->commitChanges();
                //                qDebug() << Q_FUNC_INFO << "Thread " << dbThread->toString() << " shall be updated";
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
            if (dbThread->group()->subscription()->forumId() == tempThread->group()->subscription()->forumId() &&
                    dbThread->group()->id() == tempThread->group()->id() &&
                    dbThread->id() == tempThread->id()) {
                threadFound = true;
            }
        }
        if (!threadFound) {
            deletedThreads.insert(dbThread);
            //            qDebug() << "Thread " << dbThread->toString() << " has been deleted!";
        }
    }
    foreach(ForumThread *thr, deletedThreads.values())
        thr->group()->removeThread(thr);

    group->markToBeUpdated(false);

    if(updateAll)
        updateNextChangedThread();

    fdb->checkSanity();
}

void UpdateEngine::updateThread(ForumThread *thread, bool force) {
    Q_ASSERT(fsubscription);
    Q_ASSERT(thread);
    Q_ASSERT(!thread->group()->subscription()->beingSynced());
    Q_ASSERT(!thread->group()->subscription()->scheduledForSync());

    forceUpdate = force;
    updateAll = true;
    updateOnlyThread = true;

    if(force) {
        thread->setLastPage(0);
        thread->markToBeUpdated();
    }
    if(!threadsToUpdateQueue.contains(thread))
        threadsToUpdateQueue.enqueue(thread);
    setState(PES_UPDATING);
}

void UpdateEngine::networkFailure(QString message) {
    emit updateFailure(fsubscription, "Updating " + fsubscription->alias() + " failed due to network error:\n\n" + message);
    setState(PES_ERROR);
}

void UpdateEngine::loginFinishedSlot(ForumSubscription *sub, bool success) {
    if(!success) {
        setState(PES_ERROR);
        cancelOperation();
    }
    emit loginFinished(sub, success);
    if(success)
        continueUpdate();
}

void UpdateEngine::updateNextChangedGroup() {
    Q_ASSERT(updateAll);
    Q_ASSERT(!subscription()->beingSynced());

    if(!updateOnlyThread) {
        foreach(ForumGroup *group, subscription()->values()) {
            if(group->needsToBeUpdated()) {
                doUpdateGroup(group);
                return;
            }
        }
    }
    //  qDebug() << Q_FUNC_INFO << "No more changed groups - end of update.";
    updateAll = false;
    Q_ASSERT(threadsToUpdateQueue.isEmpty());
    setState(PES_IDLE);
    emit forumUpdated(subscription());
    updateCurrentProgress();
}

void UpdateEngine::updateNextChangedThread() {
    Q_ASSERT(!subscription()->beingSynced());

    if (!threadsToUpdateQueue.isEmpty()) {
        ForumThread *thread = threadsToUpdateQueue.dequeue();
        if(forceUpdate)
            thread->setLastPage(0);
        qDebug() << Q_FUNC_INFO << threadsToUpdateQueue.size() << " threads to update in queue";
        doUpdateThread(thread);
    } else {
        updateNextChangedGroup();
    }
    updateCurrentProgress();
}

void UpdateEngine::cancelOperation() {
    if(currentState==PES_IDLE || currentState==PES_ERROR) return;
    updateAll = false;
    foreach(ForumGroup *group, subscription()->values())
        group->markToBeUpdated(false);
    threadsToUpdateQueue.clear();
    if(currentState==PES_UPDATING)
        setState(PES_IDLE);
    if(requestingCredentials)
        emit loginFinished(subscription(), false);
    requestingCredentials = false;
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
    if(newState==PES_UPDATING) {
        Q_ASSERT(oldState==PES_IDLE || oldState==PES_ERROR);
        Q_ASSERT(subscription());
        updateWhenEngineReady = false;
        if(subscription() && subscription()->authenticated()
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
    if(newState==PES_IDLE) {
        subscription()->setBeingUpdated(false);
        if(updateWhenEngineReady) {
            updateForum(forceUpdate);
            return;
        }
    }
    if(newState==PES_ENGINE_NOT_READY) {
        if(subscription())
            cancelOperation();
    }
    if(newState==PES_ERROR) {
        subscription()->setScheduledForUpdate(false);
        subscription()->setBeingUpdated(false);
        cancelOperation();
    }
    /*
    if(newState==PES_REQUESTING_CREDENTIALS) {
        if(!subscription()->authenticated() || subscription()->username().length()>0) {
            setState(PES_IDLE);
            // @todo is parser updated now?
        } // Else state changes to requesting, and Siilihai tries to provide creds.
    }
    */

    emit stateChanged(currentState, oldState);
}

void UpdateEngine::requestCredentials() {
    requestingCredentials = true;
}

void UpdateEngine::credentialsEntered(CredentialsRequest* cr) {
    requestingCredentials = false;
    if(cr->authenticator.user().length() > 0) {
        subscription()->setUsername(cr->authenticator.user());
        subscription()->setPassword(cr->authenticator.password());
        subscription()->setAuthenticated(true);
    } else {
        subscription()->setAuthenticated(false);
    }
    // Start updating when credentials entered
    if(currentState==PES_UPDATING) {
        qDebug() << Q_FUNC_INFO << "resuming update now";
        updateGroupList();
    }
}

void UpdateEngine::updateForum(bool force) {
    qDebug() << Q_FUNC_INFO << " force: " << force;
    Q_ASSERT(fsubscription);
    Q_ASSERT(!fsubscription->beingSynced());
    Q_ASSERT(!fsubscription->scheduledForSync());

    forceUpdate = force;
    updateOnlyThread = false;
    updateAll = true; // Update whole forum
    if(state()==PES_ENGINE_NOT_READY) {
        updateWhenEngineReady = true;
    } else {
        setState(PES_UPDATING);
    }
}

void UpdateEngine::updateGroupList() {
    updateAll = false; // Just the group list
    updateOnlyThread = false;
    if(state()==PES_ENGINE_NOT_READY) {
        updateWhenEngineReady = true;
    } else {
        setState(PES_UPDATING);
    }
}

void UpdateEngine::continueUpdate() {
    if(updateOnlyThread) {
        updateNextChangedThread();
    } else {
        doUpdateForum();
    }
}
