#include "parserengine.h"

ParserEngine::ParserEngine(ForumDatabase *fd, QObject *parent) :
	QObject(parent), session(this) {
    connect(&session, SIGNAL(listGroupsFinished(QList<ForumGroup*>&)), this,
            SLOT(listGroupsFinished(QList<ForumGroup*>&)));
    connect(&session,
            SIGNAL(listThreadsFinished(QList<ForumThread*>&, ForumGroup*)), this,
            SLOT(listThreadsFinished(QList<ForumThread*>&, ForumGroup*)));
    connect(&session,
            SIGNAL(listMessagesFinished(QList<ForumMessage*>&, ForumThread*)),
            this, SLOT(listMessagesFinished(QList<ForumMessage*>&, ForumThread*)));
    connect(&session,
            SIGNAL(networkFailure(QString)),
            this, SLOT(networkFailure(QString)));
    fdb = fd;
    updateAll = false;
    forceUpdate = false;
    sessionInitialized = false;
    forumBusy = 0;
}

ParserEngine::~ParserEngine() {
}

void ParserEngine::setParser(ForumParser &fp) {
    parser = fp;
}

void ParserEngine::setSubscription(ForumSubscription *fs) {
    subscription = fs;
}

void ParserEngine::updateForum(bool force) {
    qDebug() << Q_FUNC_INFO << "for forum " << parser.toString();
    Q_ASSERT(subscription);

    forceUpdate = force;
    setBusy(true);

    updateAll = true;
    if (!sessionInitialized)
        session.initialize(parser, subscription);
    session.listGroups();
    updateCurrentProgress();
}

void ParserEngine::networkFailure(QString message) {
    emit updateFailure("Updating " + subscription->alias() + " failed due to network error:\n\n" + message);
    cancelOperation();
}

void ParserEngine::updateGroupList() {
    setBusy(true);
    updateAll = false;
    if (!sessionInitialized)
        session.initialize(parser, subscription);
    session.listGroups();
    updateCurrentProgress();
}

void ParserEngine::updateNextChangedGroup() {
    Q_ASSERT(updateAll);

    if (groupsToUpdateQueue.size() > 0) {
        ForumGroup *grp=groupsToUpdateQueue.dequeue();
        Q_ASSERT(grp);
        Q_ASSERT(grp->subscribed());
        session.listThreads(grp);
    } else {
        qDebug() << "No more changed groups - end of update.";
        updateAll = false;
        Q_ASSERT(groupsToUpdateQueue.size()==0 &&
                 threadsToUpdateQueue.size()==0);
        setBusy(false);
        emit forumUpdated(subscription);
    }
    updateCurrentProgress();
}

void ParserEngine::updateNextChangedThread() {
    if (threadsToUpdateQueue.size() > 0) {
        session.listMessages(threadsToUpdateQueue.dequeue());
    } else {
        qDebug() << "PE: no more threads to update - updating next group.";
        updateNextChangedGroup();
    }
    updateCurrentProgress();
}

void ParserEngine::listGroupsFinished(QList<ForumGroup*> &groups) {
    qDebug() << Q_FUNC_INFO << " rx groups " << groups.size()
            << " in " << parser.toString();
    QList<ForumGroup*> dbgroups = fdb->listGroups(subscription);

    groupsToUpdateQueue.clear();
    if (groups.size() == 0 && dbgroups.size() > 0) {
        emit updateFailure("Updating group list for " + parser.parser_name
                           + " failed. \nCheck your network connection.");
        cancelOperation();
        return;
    }
    // Diff the group list
    bool groupsChanged = false;
    foreach(ForumGroup *grp, groups) {
        bool foundInDb = false;

        foreach(ForumGroup *dbgroup, dbgroups) {
            if (dbgroup->id() == grp->id()) {
                foundInDb = true;
                if ((dbgroup->subscribed() && ((dbgroup->lastchange()
                    != grp->lastchange()) || forceUpdate))) {
                    groupsToUpdateQueue.enqueue(dbgroup);
                    qDebug() << "Group " << dbgroup->toString()
                            << " has been changed, adding to list";
                    // Store the updated version to database
                    grp->setSubscribed(true);
                    Q_ASSERT(grp->subscription() == dbgroup->subscription());
                    dbgroup->operator=(*grp);
                    fdb->updateGroup(dbgroup);
                } else {
                    qDebug() << "Group" << dbgroup->toString()
                            << " hasn't changed or is not subscribed - not reloading.";
                }
            }
        }
        if (!foundInDb) {
            qDebug() << "Group " << grp->toString() << " not found in db - adding.";
            groupsChanged = true;
            grp->setChangeset(1);
            // DON'T set lastchange when only updating group list.
            if(!updateAll) {
                grp->setLastchange("UPDATE_NEEDED");
            }
            ForumGroup *newGroup = fdb->addGroup(grp);
            /* Don't update a group that isn't known & subscribed!
            if (updateAll) {
                groupsToUpdateQueue.enqueue(newGroup);
            }
            */
        }
    }

    // check for DELETED groups
    foreach(ForumGroup *dbgroup, dbgroups) {
        bool groupFound = false;
        foreach(ForumGroup *grp, groups) {
            if (dbgroup->id() == grp->id()) {
                groupFound = true;
            }
        }
        if (!groupFound) {
            groupsChanged = true;
            qDebug() << "Group " << dbgroup->toString() << " has been deleted!";
            fdb->deleteGroup(dbgroup);
        }
    }

    if (updateAll) {
        updateNextChangedGroup();
    } else {
        setBusy(false);
    }
    if (groupsChanged) {
        emit(groupListChanged(subscription));
    }
}

void ParserEngine::listThreadsFinished(QList<ForumThread*> &threads,
                                       ForumGroup *group) {
    qDebug() << Q_FUNC_INFO << "rx threads " << threads.size() << " in group "
            << group->toString();
    Q_ASSERT(group);
    Q_ASSERT(group->subscribed());
    QList<ForumThread*> dbthreads = fdb->listThreads(group);
    threadsToUpdateQueue.clear();
    if (threads.isEmpty() && dbthreads.size() > 0) {
        emit updateFailure("Updating thread list failed. \nCheck your network connection.");
        cancelOperation();
        return;
    }

    // Diff the group list
    bool threadsChanged = false;
    foreach(ForumThread *thr, threads) {
        bool foundInDb = false;
        foreach (ForumThread *dbthread, dbthreads) {
            Q_ASSERT(dbthread);
            if (dbthread->id() == thr->id()) {
                foundInDb = true;
                if ((dbthread->lastchange() != thr->lastchange()) || forceUpdate) {
                    Q_ASSERT(dbthread->group() == thr->group());
                    dbthread->operator=(*thr);
                    Q_ASSERT(dbthread);
                    qDebug() << Q_FUNC_INFO << "Thread " << dbthread->toString()
                            << " has been changed, updating and adding to update queue";
                    fdb->updateThread(dbthread);
                    threadsToUpdateQueue.enqueue(dbthread);
                }
            }
        }
        if (!foundInDb) {
            threadsChanged = true;
            thr->setChangeset(1);
            ForumThread *addedThread = fdb->addThread(thr);
            Q_ASSERT(addedThread);
            threadsToUpdateQueue.enqueue(addedThread);
        }
    }

    // check for DELETED threads
    foreach (ForumThread *dbthread, dbthreads) {
        bool threadFound = false;
        foreach(ForumThread *thr, threads) {
            if (dbthread->id() == thr->id()) {
                threadFound = true;
            }
        }
        if (!threadFound) {
            threadsChanged = true;
            qDebug() << "Thread " << dbthread->toString()
                    << " has been deleted!";
            fdb->deleteThread(dbthread);
        }
    }
    if(updateAll)
        updateNextChangedThread();
}

void ParserEngine::listMessagesFinished(QList<ForumMessage*> &messages,
                                        ForumThread *thread) {
    qDebug() << Q_FUNC_INFO << messages.size() << "new in " << thread->toString();
    Q_ASSERT(thread);
    Q_ASSERT(thread->group()->subscribed());
    QList<ForumMessage*> dbmessages = fdb->listMessages(thread);
    // Diff the group list
    qDebug() << "db contains " << dbmessages.size() << " msgs, got now "
            << messages.size() << " thread " << thread->toString();
    bool messagesChanged = false;
    foreach (ForumMessage *msg, messages) {
        bool foundInDb = false;
        qDebug() << Q_FUNC_INFO << "Checking new message " << msg->toString();
        foreach (ForumMessage *dbmessage, dbmessages) {
            if (dbmessage->id() == msg->id()) {
                // qDebug() << "msg id " << dbmessage.id << " & " << msg.id;
                foundInDb = true;
                //if ((dbmessage->lastchange() != msg->lastchange()) || forceUpdate) {
                //    qDebug() << "Message " << dbmessage->toString()
                //            << " has been changed.";
                qDebug() << Q_FUNC_INFO << "Updating message " << msg->toString() << " to " << dbmessage->toString();
                Q_ASSERT(msg->thread() == dbmessage->thread());
                if(dbmessage->read()) {
                    dbmessage->operator=(*msg);
                    dbmessage->setRead(true);
                } else {
                    dbmessage->operator=(*msg);
                }
                fdb->updateMessage(dbmessage);
                //}
            } else {
                qDebug() << Q_FUNC_INFO << " ..is not " << dbmessage->toString();
            }
        }
        if (!foundInDb) {
            messagesChanged = true;
            fdb->addMessage(msg);
        }
    }

    // check for DELETED threads
    foreach (ForumMessage *dbmessage, dbmessages) {
        bool messageFound = false;
        foreach (ForumMessage *msg, messages) {
            if (dbmessage->id() == msg->id()) {
                messageFound = true;
            }
        }
        if (!messageFound) {
            messagesChanged = true;
            qDebug() << "Message " << dbmessage->toString() << " has been deleted!";
            fdb->deleteMessage(dbmessage);
        }
    }
    if(updateAll)
        updateNextChangedThread();
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
    emit statusChanged(subscription, forumBusy, progress);
}

void ParserEngine::cancelOperation() {
    updateAll = false;
    session.cancelOperation();
    groupsToUpdateQueue.clear();
    threadsToUpdateQueue.clear();
    setBusy(false);
}
