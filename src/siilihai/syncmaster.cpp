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
#include "syncmaster.h"
#include "forumdata/forumsubscription.h"
#include "forumdata/forumgroup.h"
#include "forumdata/forumthread.h"
#include "forumdata/forummessage.h"
#include "forumdata/forumsubscription.h"

SyncMaster::SyncMaster(QObject *parent, ForumDatabase &fd, SiilihaiProtocol &prot) :
    QObject(parent)
  , canceled(true)
  , fdb(fd)
  , protocol(prot)
  , errorCount(0) {
    connect(&protocol, &SiilihaiProtocol::serverGroupStatus,
            this, &SyncMaster::serverGroupStatus);
    connect(&protocol, &SiilihaiProtocol::serverThreadData,
            this, &SyncMaster::serverThreadData);
    connect(&protocol, &SiilihaiProtocol::serverMessageData,
            this, &SyncMaster::serverMessageData);
    connect(&protocol, &SiilihaiProtocol::getThreadDataFinished,
            this, &SyncMaster::getThreadDataFinished);
    connect(&protocol, &SiilihaiProtocol::subscribeGroupsFinished,
            this, &SyncMaster::subscribeGroupsFinished);
    connect(&protocol, &SiilihaiProtocol::downsyncFinishedForForum,
            this, &SyncMaster::downsyncFinishedForForum);
}

SyncMaster::~SyncMaster() { }

void SyncMaster::startSync() {
    canceled = false;
    errorCount = 0;
    maxGroupCount = 0;
    for(ForumSubscription *fsub : fdb) {
        Q_ASSERT(!fsub->beingSynced());
        Q_ASSERT(!fsub->beingUpdated());
    }
    protocol.getSyncSummary();
}

void SyncMaster::endSync() {
    fdb.checkSanity();
    canceled = false;
    errorCount = 0;

    int totalGroups = 0;
    for(ForumSubscription *fsub : fdb) {
        if(fsub->hasGroupListChanged()) {
            forumsToUpload.append(fsub);
            fsub->setGroupListChanged(false);
            fsub->setBeingSynced(true);
        }
        for(ForumGroup *grp : *fsub) {
            if(grp->isSubscribed()) totalGroups++;
            if(grp->isSubscribed() && grp->hasChanged()) {
                groupsToUpload.append(grp);
            }
        }
    }
    maxGroupCount = groupsToUpload.size();
    qDebug() << Q_FUNC_INFO << "Uploading " << maxGroupCount << " of " << totalGroups << " groups.";
    processSubscriptions();
}

void SyncMaster::serverGroupStatus(const QList<ForumSubscription*> &subs) { // Temp objects!
    if(canceled) return;
    fdb.checkSanity();
    // Update local subs
    for(ForumSubscription *serverSub : subs) {
        ForumSubscription *dbSub = fdb.findById(serverSub->id());
        if(!dbSub) { // Whole forum not found in db - add it
            qDebug() << Q_FUNC_INFO << "Forum not in db -  must add it!";
            ForumSubscription *newSub = ForumSubscription::newForProvider(serverSub->provider(), &fdb, false);
            newSub->copyFrom(serverSub);
            fdb.addSubscription(newSub);
            dbSub = newSub;
        } else { // Sub already in db, just update it
            QString username, password;
            bool localIsAuthenticated = false;
            if(!dbSub->username().isEmpty()) {
                username = dbSub->username();
                password = dbSub->password();
                localIsAuthenticated = true;
            }
            dbSub->copyFrom(serverSub);
            dbSub->setUsername(username);
            dbSub->setPassword(password);
            if(localIsAuthenticated) dbSub->setAuthenticated(true);
            // UpdateEngine will get the missing credentials if
            // forum is authenticated but no u/p are known

            // Check for unsubscribed groups
            for(ForumGroup *dbGrp : *dbSub) {
                bool groupIsSubscribed = false;
                QString dbGrpId = dbGrp->id();
                for(ForumGroup *serverGrp : *serverSub) {
                    if(dbGrpId == serverGrp->id())
                        groupIsSubscribed = true;
                }
                dbGrp->setSubscribed(groupIsSubscribed);
                dbGrp->commitChanges();
            }
        }
        Q_ASSERT(dbSub);
    }
    // Check for deleted subs
    QQueue<ForumSubscription*> deletedSubs;
    for(ForumSubscription *dbSub : fdb) {
        bool found = false;
        for(ForumSubscription *serverSub : subs) {
            if(serverSub->id() == dbSub->id())
                found = true;
        }
        // Sub in db not found in sync message - delete it
        if(!found) deletedSubs.append(dbSub);
    }
    while(!deletedSubs.isEmpty())
        fdb.deleteSubscription(deletedSubs.takeFirst());

    // Update group lists
    for(ForumSubscription *serverSub : subs) {
        for(ForumGroup *serverGrp : *serverSub) {
            Q_ASSERT(serverGrp->subscription()->id() >= 0 || serverGrp->id().length() > 0);
            ForumSubscription *dbSub = fdb.findById(serverGrp->subscription()->id());
            Q_ASSERT(dbSub);

            ForumGroup *dbGroup = dbSub->value(serverGrp->id());
            if(!dbGroup) { // Group doesn't exist yet
                qDebug() << Q_FUNC_INFO << "Group " << serverGrp->toString() << " not in db -  must add it!";
                ForumGroup *newGroup = new ForumGroup(dbSub, false);
                serverGrp->setName(UNKNOWN_SUBJECT);
                serverGrp->setChangeset(-1); // Force update of group contents
                serverGrp->markToBeUpdated();
                serverGrp->setSubscribed(true);
                newGroup->copyFrom(serverGrp);
                newGroup->setChangeset(-2); // .. by setting changesets different
                dbSub->addGroup(newGroup);
                dbGroup = newGroup;
            }

            Q_ASSERT(dbGroup);

            if(dbGroup->changeset() != serverGrp->changeset()) {
                if(!dbGroup->subscription()->beingUpdated()) {
                    dbGroup->setChangeset(serverGrp->changeset());
                    groupsToDownload.append(dbGroup);
                    dbGroup->subscription()->setScheduledForSync(true);
                } else {
                    qDebug() << Q_FUNC_INFO << "NOT adding group to dl queue because it's being updated' " << dbGroup->toString();
                }
            }
            dbGroup->commitChanges();
        }
    }

    processGroups();
}

// Sends the next group in groupsToUpload and download status for
// next in groupsToDownload
void SyncMaster::processGroups() {
    fdb.checkSanity();
    if(canceled) return;
    if (groupsToUpload.isEmpty() && groupsToDownload.isEmpty()) {
        emit syncFinished(true, QString());
        return;
    }
    // Do the uploading
    if(!groupsToUpload.isEmpty()) {
        ForumGroup *g = groupsToUpload.takeFirst();
        g->subscription()->setBeingSynced(true);
        g->setChangeset(rand());
        for(ForumThread *thread : *g) {
            Q_ASSERT(thread);
            messagesToUpload.append(*thread);
        }
        connect(&protocol, SIGNAL(sendThreadDataFinished(bool, QString)), this, SLOT(sendThreadDataFinished(bool, QString)));
        protocol.sendThreadData(g, messagesToUpload);
        g->setHasChanged(false);
        g->commitChanges();
        messagesToUpload.clear();
        emit syncProgress(0, "Synchronizing " + g->name() + " in " + g->subscription()->alias() + "..");
        g->subscription()->setBeingSynced(false);
    }

    // Download groups
    if(!groupsToDownload.isEmpty()) {
        for(ForumGroup *group : groupsToDownload) {
            Q_ASSERT(!group->subscription()->beingUpdated());
            group->subscription()->setScheduledForSync(false);
            group->subscription()->setBeingSynced(true);
        }
        protocol.downsync(groupsToDownload);
    }
    fdb.checkSanity();
}

void SyncMaster::sendThreadDataFinished(bool success, QString message) {
    disconnect(&protocol, SIGNAL(sendThreadDataFinished(bool, QString)), this, SLOT(sendThreadDataFinished(bool, QString)));
    if(canceled) return;
    if (success) {
        processGroups();
    } else {
        qDebug() << Q_FUNC_INFO << "Failed! Error count: " << errorCount;
        emit syncFinished(false, message);
    }
}

void SyncMaster::serverThreadData(const ForumThread *tempThread) { // Thread is temporary object!
    if(canceled) return;
    Q_ASSERT(tempThread->group());
    Q_ASSERT(tempThread->group()->subscription());
    if (tempThread->isSane()) {
        static ForumGroup *lastGroupBeingSynced = nullptr;
        ForumGroup *dbGroup = nullptr;
        ForumThread *dbThread = fdb.getThread(tempThread->group()->subscription()->id(), tempThread->group()->id(),
                                              tempThread->id());
        if (dbThread) { // Thread already found, merge it
            dbThread->setChangeset(tempThread->changeset());
        } else { // thread hasn't been found yet!
            dbGroup = fdb.findById(tempThread->group()->subscription()->id())->value(tempThread->group()->id());
            Q_ASSERT(dbGroup);
            Q_ASSERT(!dbGroup->isTemp());
            ForumThread *newThread = new ForumThread(dbGroup, false);
            newThread->copyFrom(tempThread);
            dbGroup->addThread(newThread, false);
            newThread->markToBeUpdated();
            dbThread = newThread;
            // Make sure group will be updated
            dbGroup->markToBeUpdated();
            dbGroup->commitChanges();
        }
        dbThread->commitChanges();
        Q_ASSERT(dbThread->group()->subscription()->beingSynced());
        if(lastGroupBeingSynced != dbThread->group()) {
            dbGroup = dbThread->group();
            lastGroupBeingSynced = dbGroup;
            QString messagename;
            if(dbThread->group()->name().length() < 2) {
                messagename = "a new group";
            } else {
                messagename = "group " + dbThread->group()->name();
            }
            emit syncProgress(0, "Synchronizing " + messagename + " in " + dbThread->group()->subscription()->alias() + "..");
        }
    } else {
        qDebug() << Q_FUNC_INFO << "Got invalid thread!" << tempThread->toString();
        Q_ASSERT(false);
    }
    fdb.checkSanity();
}

void SyncMaster::serverMessageData(const ForumMessage *tempMessage) { // Temporary object!
    Q_ASSERT(tempMessage);
    if(canceled) return;
    Q_ASSERT(tempMessage->thread());
    Q_ASSERT(tempMessage->thread()->group());
    Q_ASSERT(tempMessage->thread()->group()->subscription());

    if (tempMessage->isSane()) {
        ForumMessage *dbMessage = fdb.getMessage(tempMessage->thread()->group()->subscription()->id(),
                                                 tempMessage->thread()->group()->id(), tempMessage->thread()->id(), tempMessage->id());
        if (dbMessage) { // Message already found, merge it
            dbMessage->setRead(tempMessage->isRead());
        } else { // message hasn't been found yet!
            ForumThread *dbThread = fdb.getThread(tempMessage->thread()->group()->subscription()->id(),
                                                  tempMessage->thread()->group()->id(),
                                                  tempMessage->thread()->id());
            Q_ASSERT(dbThread);
            Q_ASSERT(!dbThread->isTemp());

            // Schedule forum for update, as unknown message appears..
            dbThread->group()->subscription()->setScheduledForUpdate(true);

            ForumMessage *newMessage = new ForumMessage(dbThread, false);
            newMessage->copyFrom(tempMessage);
            newMessage->setRead(true, false);
            dbThread->addMessage(newMessage, false);
            dbThread->setLastPage(0); // Mark as 0 to force update of full thread
            newMessage->markToBeUpdated();
            dbThread->commitChanges();
            dbThread->group()->commitChanges();
            Q_ASSERT(dbThread->group()->subscription()->beingSynced());
        }
    } else {
        qDebug() << Q_FUNC_INFO << "Got invalid message!" << tempMessage->toString();
        Q_ASSERT(false);
    }
    fdb.checkSanity();
}

void SyncMaster::getThreadDataFinished(bool success, QString message){
    if(canceled) return;
    if(success) {
        for(ForumSubscription *fsub : fdb) {
            for(ForumGroup *grp : *fsub) {
                grp->commitChanges();
            }
        }
        groupsToDownload.clear();
        processGroups();
    } else {
        errorCount++;
        if(errorCount > 15) {
            for(ForumSubscription *fsub : fdb) {
                fsub->setBeingSynced(false);
            }
            emit syncFinished(false, message);
        } else {
            qDebug() << Q_FUNC_INFO << "Failed! Error count: " << errorCount;
            processGroups();
        }
    }
}

// Warning: fs is a temp object
void SyncMaster::downsyncFinishedForForum(ForumSubscription *fs)
{
    Q_ASSERT(fs && fs->id() > 0);
    ForumSubscription *dbSubscription = fdb.findById(fs->id());
    Q_ASSERT(dbSubscription);
    Q_ASSERT(!dbSubscription->scheduledForSync());
    dbSubscription->setBeingSynced(false);
    // Fix any possible sync errors:
    for(ForumGroup *grp : *dbSubscription)
        for(ForumThread *thr : *grp)
            if(thr->isEmpty()) {
                qDebug() << Q_FUNC_INFO << "Thread " << thr->toString()
                         << "contains no messages - marking to be updated";
                thr->markToBeUpdated();
            }
    emit syncFinishedFor(dbSubscription);
}


void SyncMaster::threadChanged(ForumThread *) { }

void SyncMaster::cancel() {
    // Just to make sure
    serversGroups.clear();
    serversThreads.clear();
    groupsToUpload.clear();
    groupsToDownload.clear();
    changedThreads.clear();
    forumsToUpload.clear();
    messagesToUpload.clear();

    for(ForumSubscription *fsub : fdb)
        fsub->setBeingSynced(false);

    if(!canceled) {
        canceled = true;
        // I suppose this is success..
        emit syncFinished(true, "Canceled");
    }
}

void SyncMaster::processSubscriptions() {
    if(forumsToUpload.isEmpty()) {
        processGroups();
    } else {
        ForumSubscription *sub = forumsToUpload.takeFirst();
        protocol.subscribeGroups(sub);
    }
}

void SyncMaster::subscribeGroupsFinished(bool success) {
    if(canceled) return;
    if (success) {
        processSubscriptions();
    } else {
        qDebug() << Q_FUNC_INFO << "Failed! Error count: " << errorCount;
        emit syncFinished(false, "Updating group subscriptions to server failed");
    }
}


void SyncMaster::endSyncSingleGroup(ForumGroup *group) {
    // This can happen if user reads stuff during update
    if(group->subscription()->beingUpdated() || group->subscription()->scheduledForUpdate() || group->subscription()->beingSynced() ||
            group->subscription()->scheduledForSync())
        return;

    // Ok, NEVER do this if updates are in progress
    for(ForumSubscription *fsub : fdb) {
        if(fsub->beingUpdated())
            return;
        if(fsub->beingSynced())
            return;
    }

    if(group->hasChanged()) {
        groupsToUpload.append(group);
        processGroups();
    }
}
