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
#include "forumgroup.h"
#include "forumthread.h"
#include "forummessage.h"
#include "forumdatabase.h"
#include "forumsubscription.h"
#include "parsermanager.h"

ParserEngine::ParserEngine(ForumDatabase *fd, QObject *parent, ParserManager *pm) :
    QObject(parent), nam(this), session(this, &nam), parserManager(pm) {
    connect(&session, SIGNAL(listGroupsFinished(QList<ForumGroup*>&)), this,
            SLOT(listGroupsFinished(QList<ForumGroup*>&)));
    connect(&session, SIGNAL(listThreadsFinished(QList<ForumThread*>&, ForumGroup*)), this,
            SLOT(listThreadsFinished(QList<ForumThread*>&, ForumGroup*)));
    connect(&session, SIGNAL(listMessagesFinished(QList<ForumMessage*>&, ForumThread*, bool)),
            this, SLOT(listMessagesFinished(QList<ForumMessage*>&, ForumThread*, bool)));
    connect(&session, SIGNAL(networkFailure(QString)), this, SLOT(networkFailure(QString)));
    connect(&session, SIGNAL(getAuthentication(ForumSubscription*,QAuthenticator*)),
            this, SIGNAL(getAuthentication(ForumSubscription*,QAuthenticator*)));
    connect(&session, SIGNAL(loginFinished(ForumSubscription *,bool)), this, SLOT(loginFinishedSlot(ForumSubscription *,bool)));
    connect(parserManager, SIGNAL(parserAvailable(ForumParser*)), this, SLOT(parserUpdated(ForumParser*)));
    fdb = fd;
    updateAll = false;
    forceUpdate = false;
    sessionInitialized = false;
    fsubscription = 0;
    currentState = PES_MISSING_PARSER;
}

ParserEngine::~ParserEngine() {
    if(fsubscription)
        fsubscription->setParserEngine(0);
}

void ParserEngine::setParser(ForumParser *fp) {
    currentParser = fp;
    if(fsubscription) setState(PES_IDLE);
}

void ParserEngine::setSubscription(ForumSubscription *fs) {
    Q_ASSERT(!fsubscription); // Don't reuse this class, plz!
    fsubscription = fs;
    connect(fsubscription, SIGNAL(destroyed()), this, SLOT(subscriptionDeleted()));
    fsubscription->setParserEngine(this);

    parserManager->updateParser(fsubscription->parser());
    setState(PES_UPDATING_PARSER);
}

void ParserEngine::updateForum(bool force) {
    if(!currentParser->isSane()) qDebug() << Q_FUNC_INFO << "Warning: Parser not sane!";
    Q_ASSERT(fsubscription);

    forceUpdate = force;
    updateAll = true;
    if (!sessionInitialized) {
        session.initialize(currentParser, fsubscription);
        sessionInitialized = true;
    }
    session.listGroups();
    setState(PES_UPDATING);
}

void ParserEngine::updateGroupList() {
    if(!currentParser->isSane()) qDebug() << Q_FUNC_INFO << "Warning: Parser not sane!";
    updateAll = false;
    if (!sessionInitialized) {
        session.initialize(currentParser, fsubscription);
        sessionInitialized = true;
    }

    session.listGroups();
    setState(PES_UPDATING);
}

void ParserEngine::updateThread(ForumThread *thread, bool force) {
    Q_ASSERT(fsubscription);
    Q_ASSERT(thread);
    if(!currentParser->isSane()) qDebug() << Q_FUNC_INFO << "Warning: Parser not sane!";

    forceUpdate = false;
    updateAll = false;

    if (!sessionInitialized) {
        session.initialize(currentParser, fsubscription);
        sessionInitialized = true;
    }
    if(force) {
        thread->setLastPage(0);
        thread->markToBeUpdated();
    }
    session.listMessages(thread);
    setState(PES_UPDATING);
}

void ParserEngine::networkFailure(QString message) {
    emit updateFailure(fsubscription, "Updating " + fsubscription->alias() + " failed due to network error:\n\n" + message);
    setState(PES_ERROR);
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
        Q_ASSERT(groupsToUpdateQueue.isEmpty() &&threadsToUpdateQueue.isEmpty());
        setState(PES_IDLE);
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
        updateNextChangedGroup();
    }
    updateCurrentProgress();
}

void ParserEngine::listGroupsFinished(QList<ForumGroup*> &tempGroups) {
    bool dbGroupsWasEmpty = fsubscription->isEmpty();
    groupsToUpdateQueue.clear();
    fsubscription->markToBeUpdated(false);
    if (tempGroups.size() == 0 && fsubscription->size() > 0) {
        emit updateFailure(fsubscription, "Updating group list for " + currentParser->parser_name
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
                    groupsToUpdateQueue.enqueue(dbGroup);
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
            if(!updateAll) {
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
            qDebug() << "Group " << dbGroup->toString() << " has been deleted!";
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
}

void ParserEngine::listThreadsFinished(QList<ForumThread*> &tempThreads, ForumGroup *group) {
    Q_ASSERT(group);
    Q_ASSERT(group->isSubscribed());
    Q_ASSERT(!group->isTemp());
    Q_ASSERT(group->isSane());
    threadsToUpdateQueue.clear();
    group->markToBeUpdated(false);
    if (tempThreads.isEmpty() && !group->isEmpty()) {
        QString errorMsg = "Found no threads in group " + group->toString() + ".\n Broken parser?";
        emit updateFailure(fsubscription, errorMsg);
        group->markToBeUpdated();
        setState(PES_ERROR);
        return;
    }

    // Diff the group list
    foreach(ForumThread *serverThread, tempThreads) {
        ForumThread *dbThread = fdb->getThread(group->subscription()->parser(), group->id(), serverThread->id());
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
                qDebug() << Q_FUNC_INFO << "Thread " << dbThread->toString() << " shall be updated";
                dbThread->markToBeUpdated();
                threadsToUpdateQueue.enqueue(dbThread);
            }
        } else {
            ForumThread *newThread = new ForumThread(group, false);
            newThread->copyFrom(serverThread);
            newThread->setChangeset(-1);
            group->addThread(newThread);
            threadsToUpdateQueue.enqueue(newThread);
        }
    }
    QSet<ForumThread*> deletedThreads;
    // check for DELETED threads
    foreach (ForumThread *dbThread, group->values()) { // Iterate all db threads and find if any is missing
        bool threadFound = false;
        foreach(ForumThread *tempThread, tempThreads) {
            if (dbThread->group()->subscription()->parser() == tempThread->group()->subscription()->parser() &&
                    dbThread->group()->id() == tempThread->group()->id() &&
                    dbThread->id() == tempThread->id()) {
                threadFound = true;
            }
        }
        if (!threadFound) {
            deletedThreads.insert(dbThread);
            qDebug() << "Thread " << dbThread->toString() << " has been deleted!";
        }
    }
    foreach(ForumThread *thr, deletedThreads.values())
        thr->group()->removeThread(thr);

    if(updateAll)
        updateNextChangedThread();
}

void ParserEngine::listMessagesFinished(QList<ForumMessage*> &tempMessages, ForumThread *dbThread, bool moreAvailable) {
    Q_ASSERT(dbThread);
    Q_ASSERT(dbThread->isSane());
    Q_ASSERT(!dbThread->isTemp());
    Q_ASSERT(dbThread->group()->isSubscribed());
    dbThread->markToBeUpdated(false);
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
            if (dbThread->group()->subscription()->parser() == msg->thread()->group()->subscription()->parser() &&
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
        emit forumUpdated(fsubscription);
    }
}

void ParserEngine::updateCurrentProgress() {
    float progress = -1;
    emit statusChanged(fsubscription, progress);
}

void ParserEngine::cancelOperation() {
    if(currentState==PES_IDLE) return;
    updateAll = false;
    session.cancelOperation();
    groupsToUpdateQueue.clear();
    threadsToUpdateQueue.clear();
    if(currentState==PES_UPDATING)
        setState(PES_IDLE);
}

void ParserEngine::loginFinishedSlot(ForumSubscription *sub, bool success) {
    if(!success) {
        setState(PES_ERROR);
        cancelOperation();
    }
    emit loginFinished(sub, success);
}

ForumSubscription* ParserEngine::subscription() {
    return fsubscription;
}

void ParserEngine::subscriptionDeleted() {
    cancelOperation();
    fsubscription = 0;
    setState(PES_MISSING_PARSER);
}

QNetworkAccessManager * ParserEngine::networkAccessManager() {
    return &nam;
}

void ParserEngine::setState(ParserEngineState newState) {
    if(newState == currentState) return;
    ParserEngineState oldState = currentState;
    currentState = newState;
    if(newState==PES_UPDATING) {
        Q_ASSERT(oldState==PES_IDLE || oldState==PES_ERROR);
    }
    if(newState==PES_UPDATING_PARSER) {
        Q_ASSERT(oldState==PES_IDLE || oldState==PES_MISSING_PARSER);
    }

    if(newState==PES_ERROR) {
        cancelOperation();
    }
    emit stateChanged(currentState);
}

ParserEngine::ParserEngineState ParserEngine::state() {
    return currentState;
}

ForumParser *ParserEngine::parser() {
    return currentParser;
}

void ParserEngine::parserUpdated(ForumParser *p) {
    Q_ASSERT(currentState==PES_UPDATING_PARSER);
    if(subscription()->parser() == parser()->id) {
        setParser(p);
    }
}
