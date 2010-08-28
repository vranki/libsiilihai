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
#include "parserengine.h"

ParserEngine::ParserEngine(ForumDatabase *fd, QObject *parent) :
	QObject(parent), session(this) {
    connect(&session, SIGNAL(listGroupsFinished(QList<ForumGroup*>&)), this,
            SLOT(listGroupsFinished(QList<ForumGroup*>&)));
    connect(&session,
            SIGNAL(listThreadsFinished(QList<ForumThread*>&, ForumGroup*)), this,
            SLOT(listThreadsFinished(QList<ForumThread*>&, ForumGroup*)));
    connect(&session,
            SIGNAL(listMessagesFinished(QList<ForumMessage*>&, ForumThread*, bool)),
            this, SLOT(listMessagesFinished(QList<ForumMessage*>&, ForumThread*, bool)));
    connect(&session,
            SIGNAL(networkFailure(QString)),
            this, SLOT(networkFailure(QString)));
    connect(&session,
            SIGNAL(getAuthentication(ForumSubscription*,QAuthenticator*)),
            this, SIGNAL(getAuthentication(ForumSubscription*,QAuthenticator*)));

    connect(&session, SIGNAL(loginFinished(ForumSubscription *,bool)), this, SLOT(loginFinishedSlot(ForumSubscription *,bool)));
    fdb = fd;
    updateAll = false;
    forceUpdate = false;
    sessionInitialized = false;
    forumBusy = false;
}

ParserEngine::~ParserEngine() {
}

void ParserEngine::setParser(ForumParser &fp) {
    parser = fp;
}

void ParserEngine::setSubscription(ForumSubscription *fs) {
    fsubscription = fs;
}

void ParserEngine::updateForum(bool force) {
   // qDebug() << Q_FUNC_INFO << "for forum " << parser.toString();
    Q_ASSERT(fsubscription);

    forceUpdate = force;
    updateAll = true;
    if (!sessionInitialized) {
        session.initialize(parser, fsubscription);
        sessionInitialized = true;
    }
    session.listGroups();
    setBusy(true);
}

void ParserEngine::updateGroupList() {
    updateAll = false;
    if (!sessionInitialized) {
        session.initialize(parser, fsubscription);
        sessionInitialized = true;
    }

    session.listGroups();
    setBusy(true);
}

void ParserEngine::updateThread(ForumThread *thread) {
    Q_ASSERT(fsubscription);
    Q_ASSERT(thread);
    // qDebug() << Q_FUNC_INFO << "for thread " << thread->toString();

    forceUpdate = false;
    updateAll = false;

    if (!sessionInitialized) {
        session.initialize(parser, fsubscription);
        sessionInitialized = true;
    }

    session.listMessages(thread);
    setBusy(true);
}

void ParserEngine::networkFailure(QString message) {
    emit updateFailure(fsubscription, "Updating " + fsubscription->alias() + " failed due to network error:\n\n" + message);
    cancelOperation();
}


void ParserEngine::updateNextChangedGroup() {
    Q_ASSERT(updateAll);

    if (groupsToUpdateQueue.size() > 0) {
        ForumGroup *grp=groupsToUpdateQueue.dequeue();
        Q_ASSERT(grp);
        Q_ASSERT(grp->subscribed());
        session.listThreads(grp);
    } else {
      //  qDebug() << Q_FUNC_INFO << "No more changed groups - end of update.";
        updateAll = false;
        Q_ASSERT(groupsToUpdateQueue.size()==0 &&
                 threadsToUpdateQueue.size()==0);
        setBusy(false);
        emit forumUpdated(fsubscription);
    }
    updateCurrentProgress();
}

void ParserEngine::updateNextChangedThread() {
    if (threadsToUpdateQueue.size() > 0) {
        session.listMessages(threadsToUpdateQueue.dequeue());
    } else {
      //  qDebug() << "PE: no more threads to update - updating next group.";
        updateNextChangedGroup();
    }
    updateCurrentProgress();
}

void ParserEngine::listGroupsFinished(QList<ForumGroup*> &tempGroups) {
   // qDebug() << Q_FUNC_INFO << " rx groups " << groups.size()
   //         << " in " << parser.toString();
  //  QList<ForumGroup*> dbgroups = fdb->listGroups(fsubscription);
    bool dbGroupsWasEmpty = fsubscription->groups().isEmpty();
    groupsToUpdateQueue.clear();
    if (tempGroups.size() == 0 && fsubscription->groups().size() > 0) {
        emit updateFailure(fsubscription, "Updating group list for " + parser.parser_name
                           + " failed. \nCheck your network connection.");
        cancelOperation();
        return;
    }
    // Diff the group list
    bool groupsChanged = false;
    foreach(ForumGroup *tempGroup, tempGroups) {
        bool foundInDb = false;

        foreach(ForumGroup *dbGroup, fsubscription->groups()) {
            if (dbGroup->id() == tempGroup->id()) {
                foundInDb = true;
                if ((dbGroup->subscribed() && ((dbGroup->lastchange()
                    != tempGroup->lastchange()) || forceUpdate))) {
                    groupsToUpdateQueue.enqueue(dbGroup);
                    qDebug() << "Group " << dbGroup->toString()
                            << " has been changed, adding to list";
                    // Store the updated version to database
                    tempGroup->setSubscribed(true);
                    Q_ASSERT(tempGroup->subscription() == dbGroup->subscription());
                    dbGroup->copyFrom(tempGroup);
                } else {
           //         qDebug() << "Group" << dbgroup->toString()
            //                << " hasn't changed or is not subscribed - not reloading.";
                }
            }
        }
        if (!foundInDb) {
            // qDebug() << "Group " << grp->toString() << " not found in db - adding.";
            groupsChanged = true;
            ForumGroup *newGroup = new ForumGroup(fsubscription);
            newGroup->copyFrom(tempGroup);
            newGroup->setChangeset(rand());
            // DON'T set lastchange when only updating group list.
            if(!updateAll) {
                newGroup->setLastchange("UPDATE_NEEDED");
            }
            fdb->addGroup(newGroup);
            /* Don't update a group that isn't known & subscribed!
            if (updateAll) {
                groupsToUpdateQueue.enqueue(newGroup);
            }
            */
        }
    }

    // check for DELETED groups
    foreach(ForumGroup *dbGroup, fsubscription->groups()) {
        bool groupFound = false;
        foreach(ForumGroup *grp, tempGroups) {
            if (dbGroup->id() == grp->id()) {
                groupFound = true;
            }
        }
        if (!groupFound) {
            groupsChanged = true;
            qDebug() << "Group " << dbGroup->toString() << " has been deleted!";
            fdb->deleteGroup(dbGroup);
        }
    }

    if (updateAll) {
        updateNextChangedGroup();
    } else {
        setBusy(false);
    }
    if (groupsChanged && dbGroupsWasEmpty) {
        emit(groupListChanged(fsubscription));
    }
}

void ParserEngine::listThreadsFinished(QList<ForumThread*> &tempThreads,
                                       ForumGroup *group) {
   // qDebug() << Q_FUNC_INFO << "rx threads " << threads.size() << " in group "
      //      << group->toString();
    Q_ASSERT(group);
    Q_ASSERT(group->subscribed());
    Q_ASSERT(!group->isTemp());
    Q_ASSERT(group->isSane());
    threadsToUpdateQueue.clear();
    if (tempThreads.isEmpty() && group->threads().size() > 0) {
        emit updateFailure(fsubscription, "Updating thread list failed. \nCheck your network connection.");
        cancelOperation();
        return;
    }

    // Diff the group list
    foreach(ForumThread *serverThread, tempThreads) {
        ForumThread *dbThread = fdb->getThread(group->subscription()->parser(), group->id(), serverThread->id());
        if (dbThread) {
            dbThread->setName(serverThread->name());
            if ((dbThread->lastchange() != serverThread->lastchange()) || forceUpdate) {
                Q_ASSERT(dbThread->group() == serverThread->group());

                // Don't update some fields to new values
                int oldGetMessagesCount = dbThread->getMessagesCount();
                bool oldHasMoreMessages =  dbThread->hasMoreMessages();
                dbThread->setLastchange(serverThread->lastchange());
                dbThread->setOrdernum(serverThread->ordernum());
                dbThread->setChangeset(serverThread->changeset());
                dbThread->setGetMessagesCount(oldGetMessagesCount);
                dbThread->setHasMoreMessages(oldHasMoreMessages);

                qDebug() << Q_FUNC_INFO << "Thread " << dbThread->toString()
                        << " has been changed, updating and adding to update queue";
                threadsToUpdateQueue.enqueue(dbThread);
            }
        } else {
            ForumThread *newThread = new ForumThread(group, false);
            newThread->copyFrom(serverThread);
            newThread->setChangeset(-1);
            fdb->addThread(newThread);
            threadsToUpdateQueue.enqueue(newThread);
        }
    }

    // check for DELETED threads
    foreach (ForumThread *dbThread, group->threads()) { // Iterate all db threads and find if any is missing
        bool threadFound = false;
        foreach(ForumThread *tempThread, tempThreads) {
            if (dbThread->id() == tempThread->id()) {
                threadFound = true;
            }
        }
        if (!threadFound) {
            qDebug() << "Thread " << dbThread->toString()
                    << " has been deleted!";
            fdb->deleteThread(dbThread);
        }
    }
    if(updateAll)
        updateNextChangedThread();
}

void ParserEngine::listMessagesFinished(QList<ForumMessage*> &tempMessages,
                                        ForumThread *thread, bool moreAvailable) {
   // qDebug() << Q_FUNC_INFO << messages.size() << "new in " << thread->toString()
   //         << " more available: " << moreAvailable;
    Q_ASSERT(thread);
    Q_ASSERT(thread->isSane());
    Q_ASSERT(!thread->isTemp());
    Q_ASSERT(thread->group()->subscribed());
    //QList<ForumMessage*> dbmessages = fdb->listMessages(thread);
    // Diff the group list
   // qDebug() << "db contains " << dbmessages.size() << " msgs, got now "
  //          << messages.size() << " thread " << thread->toString();
    bool messagesChanged = false;
    foreach (ForumMessage *tempMessage, tempMessages) {
        bool foundInDb = false;
        foreach (ForumMessage *dbMessage, thread->messages()) {
            if (dbMessage->id() == tempMessage->id()) {
                // qDebug() << "msg id " << dbmessage.id << " & " << msg.id;
                foundInDb = true;
                //if ((dbmessage->lastchange() != msg->lastchange()) || forceUpdate) {
                //    qDebug() << "Message " << dbmessage->toString()
                //            << " has been changed.";
            //    qDebug() << Q_FUNC_INFO << "Updating message " << msg->toString() << " to " << dbmessage->toString();
                Q_ASSERT(tempMessage->thread() == dbMessage->thread());
                bool wasRead = dbMessage->read();
                dbMessage->copyFrom(tempMessage);
                if(wasRead) dbMessage->setRead(true);
            }
        }
        if (!foundInDb) {
            messagesChanged = true;
            ForumMessage *newMessage = new ForumMessage(thread, false);
            newMessage->copyFrom(tempMessage);
            fdb->addMessage(newMessage);
        }
    }

    // check for DELETED threads
    foreach (ForumMessage *dbmessage, thread->messages()) {
        bool messageFound = false;
        foreach (ForumMessage *msg, tempMessages) {
            if (dbmessage->id() == msg->id()) {
                messageFound = true;
            }
        }
        if (!messageFound) {
            messagesChanged = true;
        //    qDebug() << "Message " << dbmessage->toString() << " has been deleted!";
            fdb->deleteMessage(dbmessage);
        }
    }
    // update thread
    thread->setHasMoreMessages(moreAvailable);

    if(updateAll) {
        updateNextChangedThread();
    } else {
        setBusy(false);
        emit forumUpdated(fsubscription);
    }
}

bool ParserEngine::isBusy() {
    return forumBusy;
}

void ParserEngine::setBusy(bool busy) {
    if (busy != forumBusy) {
        forumBusy = busy;
        updateCurrentProgress();
    }
    forumBusy = busy;
}

void ParserEngine::updateCurrentProgress() {
    float progress = -1;
    emit statusChanged(fsubscription, forumBusy, progress);
}

void ParserEngine::cancelOperation() {
    updateAll = false;
    session.cancelOperation();
    groupsToUpdateQueue.clear();
    threadsToUpdateQueue.clear();
    setBusy(false);
}

void ParserEngine::loginFinishedSlot(ForumSubscription *sub, bool success) {
    if(!success)
        cancelOperation();

    emit loginFinished(sub, success);
}

ForumSubscription* ParserEngine::subscription() {
    return fsubscription;
}
