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

SyncMaster::SyncMaster(QObject *parent, ForumDatabase &fd,
                       SiilihaiProtocol &prot) :
QObject(parent), fdb(fd), protocol(prot) {

    connect(&protocol, SIGNAL(serverGroupStatus(QList<ForumSubscription*> &)), this,
            SLOT(serverGroupStatus(QList<ForumSubscription*> &)));
    connect(&protocol, SIGNAL(serverThreadData(ForumThread*)), this,
            SLOT(serverThreadData(ForumThread*)));
    connect(&protocol, SIGNAL(serverMessageData(ForumMessage*)), this,
            SLOT(serverMessageData(ForumMessage*)));
    connect(&protocol, SIGNAL(getThreadDataFinished(bool, QString)), this,
            SLOT(getThreadDataFinished(bool, QString)));
    canceled = true;
    groupBeingDownloaded = 0;
    errorCount = 0;
}

SyncMaster::~SyncMaster() {
}

void SyncMaster::startSync() {
    qDebug( ) << Q_FUNC_INFO;
    canceled = false;
    errorCount = 0;
    groupBeingDownloaded = 0;
    maxGroupCount = 0;
    protocol.getSyncSummary();
}

void SyncMaster::endSync() {
    qDebug( ) << Q_FUNC_INFO;
    canceled = false;
    errorCount = 0;

    QList<ForumSubscription*> fsubs = fdb.listSubscriptions();
    int totalGroups = 0;
    foreach(ForumSubscription *fsub, fsubs) {
        foreach(ForumGroup *grp, *fsub) {
            if(grp->subscribed()) totalGroups++;
            if(grp->subscribed() && grp->hasChanged()) {
               groupsToUpload.append(grp);
            }
         }
    }
    maxGroupCount = groupsToUpload.size();
    qDebug() << Q_FUNC_INFO << "Uploading " << maxGroupCount << " of " << totalGroups << " groups.";
    processGroups();
}

void SyncMaster::serverGroupStatus(QList<ForumSubscription*> &subs) {
    qDebug() << Q_FUNC_INFO << subs.size();
    //maxGroupCount = grps.size();
    // Make a list of updated subscriptions
    /*
    QSet<ForumSubscription*> updatedSubs;
    foreach(ForumGroup grp, grps) {
        ForumSubscription *fs = grp.subscription();
        updatedSubs.insert(fs);
    }
*/
    // Update local subs
    foreach(ForumSubscription *serverSub, subs) {
        ForumSubscription *dbSub = fdb.getSubscription(serverSub->parser());
        if(!dbSub) { // Whole forum not found in db - add it
            qDebug() << Q_FUNC_INFO << "Forum not in db -  must add it!";
            ForumSubscription *newSub = new ForumSubscription(&fdb);
            newSub->setParser(serverSub->parser());
            newSub->setAlias(serverSub->alias());
            newSub->setLatestThreads(serverSub->latest_threads());
            newSub->setLatestMessages(serverSub->latest_messages());
            newSub->setAuthenticated(serverSub->authenticated());
            fdb.addSubscription(newSub);
            dbSub = newSub;
        } else {
            dbSub->setParser(serverSub->parser());
            dbSub->setAlias(serverSub->alias());
            dbSub->setLatestThreads(serverSub->latest_threads());
            dbSub->setLatestMessages(serverSub->latest_messages());
            dbSub->setAuthenticated(serverSub->authenticated());
            // Check for unsubscribed groups
            foreach(ForumGroup *dbGrp, *dbSub) {
                bool found = false;
                foreach(ForumGroup *serverGrp, *serverSub) {
                    if(dbGrp->id() == serverGrp->id())
                        found = true;
                }
                if(!found) {
                    dbGrp->setSubscribed(false);
                }
            }
        }

        Q_ASSERT(dbSub);
    }

    foreach(ForumSubscription *serverSub, subs) {
        foreach(ForumGroup *serverGrp, *serverSub) {
            Q_ASSERT(serverGrp->subscription()->parser() >= 0 || serverGrp->id().length() > 0);
            ForumSubscription *dbSub = fdb.getSubscription(serverGrp->subscription()->parser());
            Q_ASSERT(dbSub);

            ForumGroup *dbGroup = fdb.getGroup(dbSub, serverGrp->id());
            if(!dbGroup) { // Group doesn't exist yet
                qDebug() << Q_FUNC_INFO << "Group " << serverGrp->toString() << " not in db -  must add it!";
                ForumGroup *group = new ForumGroup(dbSub);
                group->setName("?");
                group->setId(serverGrp->id());
                group->setChangeset(-1); // Force update of group contents
                group->setSubscribed(true);
                fdb.addGroup(group);
                dbGroup = group;
            }

            Q_ASSERT(dbGroup);

            qDebug() << Q_FUNC_INFO << "Changesets for " << dbGroup->toString() << ": db " << dbGroup->changeset() << " server " << serverGrp->changeset();
            if(dbGroup->changeset() != serverGrp->changeset()) {
                qDebug() << "Adding group to download queue and setting changeset " << dbGroup->toString();
                dbGroup->setChangeset(serverGrp->changeset());
                groupsToDownload.append(dbGroup);
            }
        }
    }

    processGroups();
}

// Sends the next group in groupsToUpload and download status for
// next in groupsToDownload
void SyncMaster::processGroups() {
    //qDebug( ) << Q_FUNC_INFO;
    if(canceled) return;
    emit syncProgress((float) (maxGroupCount - groupsToUpload.size() - groupsToDownload.size()) / (float) (maxGroupCount+1));
    if (groupsToUpload.isEmpty() && groupsToDownload.isEmpty()) {
        emit syncFinished(true, QString::null);
        return;
    }
    // Do the uploading
    if(!groupsToUpload.isEmpty()) {
        ForumGroup *g = groupsToUpload.takeFirst();
        g->setChangeset(rand());
        qDebug() << Q_FUNC_INFO << "Group " << g->toString() << "new changeset: " << g->changeset();
        foreach(ForumThread *thread, *g)
        {
            messagesToUpload.append(*thread);
        }
        connect(&protocol, SIGNAL(sendThreadDataFinished(bool, QString)),
                this, SLOT(sendThreadDataFinished(bool, QString)));
        protocol.sendThreadData(g, messagesToUpload);
        messagesToUpload.clear();
    }
    if(!groupsToDownload.isEmpty()) {
        Q_ASSERT(!groupBeingDownloaded);
        groupBeingDownloaded = groupsToDownload.takeFirst();
        //qDebug() << Q_FUNC_INFO << " will now D/L group " << groupBeingDownloaded->toString();
        protocol.getThreadData(groupBeingDownloaded);
    }
}

void SyncMaster::sendThreadDataFinished(bool success, QString message) {
    qDebug() << Q_FUNC_INFO << success;
    disconnect(&protocol, SIGNAL(sendThreadDataFinished(bool, QString)),
               this, SLOT(sendThreadDataFinished(bool, QString)));
    if(canceled) return;
    if (success) {
        processGroups();
    } else {
        qDebug() << Q_FUNC_INFO << "Failed! Error count: " << errorCount;
        emit syncFinished(false, message);
    }
}

void SyncMaster::serverThreadData(ForumThread *thread) {
    // qDebug() << Q_FUNC_INFO << thread->toString();
    if(canceled) return;
    if (thread->isSane()) {
        ForumThread *dbThread = fdb.getThread(thread->group()->subscription()->parser(), thread->group()->id(),
                                              thread->id());
        if (dbThread) { // Thread already found, merge it
            dbThread->setChangeset(thread->changeset());
        } else { // thread hasn't been found yet!
            ForumGroup *dbGroup = fdb.getGroup(fdb.getSubscription(thread->group()->subscription()->parser()), thread->group()->id());
            Q_ASSERT(dbGroup);
            thread->setGroup(dbGroup);
            fdb.addThread(thread);
            // Make sure group will be updated
            if(dbGroup->lastchange()!="UPDATE_NEEDED") {
                dbGroup->setLastchange("UPDATE_NEEDED");
            }
        }
    } else {
        qDebug() << "Got invalid thread!" << thread->toString();
        Q_ASSERT(false);
    }
}

void SyncMaster::serverMessageData(ForumMessage *message) {
    qDebug() << Q_FUNC_INFO << message->toString();
    if(canceled) return;
    if (message->isSane()) {
        ForumMessage *dbMessage = fdb.getMessage(message->thread()->group()->subscription()->parser(),
                                                 message->thread()->group()->id(), message->thread()->id(), message->id());
        if (dbMessage) { // Message already found, merge it
            dbMessage->setRead(message->read());
        } else { // message hasn't been found yet!
            ForumThread *dbThread = fdb.getThread(message->thread()->group()->subscription()->parser(),
                                                  message->thread()->group()->id(),
                                                  message->thread()->id());
            Q_ASSERT(dbThread);
            message->setThread(dbThread);
            fdb.addMessage(message);
        }
    } else {
        qDebug() << Q_FUNC_INFO << "Got invalid message!" << message->toString();
        Q_ASSERT(false);
    }
}

void SyncMaster::getThreadDataFinished(bool success, QString message){
    qDebug() << Q_FUNC_INFO << success;
    if(canceled) return;
    if(success) {
        groupBeingDownloaded = 0;
        processGroups();
    } else {
        errorCount++;
        if(errorCount > 15) {
            groupBeingDownloaded = 0;
            emit syncFinished(false, message);
        } else {
            qDebug() << Q_FUNC_INFO << "Failed! Error count: " << errorCount;
            Q_ASSERT(groupBeingDownloaded);
            groupsToDownload.append(groupBeingDownloaded);
            groupBeingDownloaded = 0;
            processGroups();
        }
    }
}

void SyncMaster::threadChanged(ForumThread *thread) {
    qDebug() << Q_FUNC_INFO << thread->toString();
}

void SyncMaster::cancel() {
    serversGroups.clear();
    serversThreads.clear();
    groupsToUpload.clear();
    groupsToDownload.clear();
    changedThreads.clear();
    forumsToUpload.clear();
    messagesToUpload.clear();
    emit syncFinished(false, "Canceled");
}
