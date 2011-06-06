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
    QObject(parent), nam(this), session(this, &nam) {
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
    fsubscription = 0;
}

ParserEngine::~ParserEngine() {
    if(fsubscription)
        fsubscription->setParserEngine(0);
}

void ParserEngine::setParser(ForumParser &fp) {
    parser = fp;
}

void ParserEngine::setSubscription(ForumSubscription *fs) {
    Q_ASSERT(!fsubscription); // Don't reuse this class, plz!
    fsubscription = fs;
    connect(fsubscription, SIGNAL(destroyed()), this, SLOT(subscriptionDeleted()));
    fsubscription->setParserEngine(this);
}

void ParserEngine::updateForum(bool force) {
    if(!parser.isSane()) qDebug() << Q_FUNC_INFO << "Warning: Parser not sane!";
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
    if(!parser.isSane()) qDebug() << Q_FUNC_INFO << "Warning: Parser not sane!";
    updateAll = false;
    if (!sessionInitialized) {
        session.initialize(parser, fsubscription);
        sessionInitialized = true;
    }

    session.listGroups();
    setBusy(true);
}

void ParserEngine::updateThread(ForumThread *thread, bool force) {
    Q_ASSERT(fsubscription);
    Q_ASSERT(thread);
    if(!parser.isSane()) qDebug() << Q_FUNC_INFO << "Warning: Parser not sane!";

    forceUpdate = false;
    updateAll = false;

    if (!sessionInitialized) {
        session.initialize(parser, fsubscription);
        sessionInitialized = true;
    }
    if(force) {
        thread->setLastPage(0);
        thread->markToBeUpdated();
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
        Q_ASSERT(grp->isSubscribed());
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
    if (!threadsToUpdateQueue.isEmpty()) {
        ForumThread *thread = threadsToUpdateQueue.dequeue();
        if(forceUpdate)
            thread->setLastPage(0);
        session.listMessages(thread);
    } else {
        //  qDebug() << "PE: no more threads to update - updating next group.";
        updateNextChangedGroup();
    }
    updateCurrentProgress();
}

void ParserEngine::listGroupsFinished(QList<ForumGroup*> &tempGroups) {
    // qDebug() << Q_FUNC_INFO << " rx groups " << groups.size()
    //         << " in " << parser.toString();
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
                if((dbGroup->isSubscribed() &&
                    ((dbGroup->lastchange() != tempGroup->lastchange()) || forceUpdate))) {
                    groupsToUpdateQueue.enqueue(dbGroup);
                    qDebug() << Q_FUNC_INFO << "Group " << dbGroup->toString()
                             << " has been changed, adding to list";
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
            if(!updateAll) {
                newGroup->markToBeUpdated();
            }
            fdb->addGroup(newGroup);
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
    Q_ASSERT(group->isSubscribed());
    Q_ASSERT(!group->isTemp());
    Q_ASSERT(group->isSane());
    threadsToUpdateQueue.clear();
    fdb->checkSanity();
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
                dbThread->commitChanges();
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
            fdb->checkSanity();
            qDebug() << "Thread " << dbThread->toString() << " has been deleted!";
            fdb->deleteThread(dbThread);
        }
    }
    if(updateAll)
        updateNextChangedThread();
}

void ParserEngine::listMessagesFinished(QList<ForumMessage*> &tempMessages, ForumThread *dbThread,
                                        bool moreAvailable) {
    Q_ASSERT(dbThread);
    Q_ASSERT(dbThread->isSane());
    Q_ASSERT(!dbThread->isTemp());
    Q_ASSERT(dbThread->group()->isSubscribed());

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
                dbMessage->commitChanges();
            }
        }
        if (!foundInDb) {
            messagesChanged = true;
            ForumMessage *newMessage = new ForumMessage(dbThread, false);
            newMessage->copyFrom(tempMessage);
            dbThread->addMessage(newMessage);
        }
    }

    // check for DELETED threads
    foreach (ForumMessage *dbmessage, dbThread->values()) {
        bool messageFound = false;
        foreach (ForumMessage *msg, tempMessages) {
            if (dbmessage->id() == msg->id()) {
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

void ParserEngine::subscriptionDeleted() {
    cancelOperation();
    fsubscription = 0;
}
QNetworkAccessManager * ParserEngine::networkAccessManager() {
    return &nam;
}
