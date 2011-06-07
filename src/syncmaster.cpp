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
    fdb.checkSanity();
    canceled = false;
    errorCount = 0;

    QList<ForumSubscription*> fsubs = fdb.listSubscriptions();
    int totalGroups = 0;
    foreach(ForumSubscription *fsub, fsubs) {
        qDebug() << Q_FUNC_INFO << fsub;
        foreach(ForumGroup *grp, fsub->groups()) {
            qDebug() << Q_FUNC_INFO << grp;
            if(grp->isSubscribed()) totalGroups++;
            if(grp->isSubscribed() && grp->hasChanged()) {
                groupsToUpload.append(grp);
            }
        }
    }
    maxGroupCount = groupsToUpload.size();
    qDebug() << Q_FUNC_INFO << "Uploading " << maxGroupCount << " of " << totalGroups << " groups.";
    processGroups();
}

void SyncMaster::serverGroupStatus(QList<ForumSubscription*> &subs) { // Temp objects!
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
            ForumSubscription *newSub = new ForumSubscription(&fdb, false);
            newSub->copyFrom(serverSub);
            fdb.addSubscription(newSub);
            dbSub = newSub;
        } else { // Sub already in db, just update it
            dbSub->copyFrom(serverSub);
            // Check for unsubscribed groups
            foreach(ForumGroup *dbGrp, dbSub->groups()) {
                bool groupIsSubscribed = false;
                foreach(ForumGroup *serverGrp, serverSub->groups()) {
                    if(dbGrp->id() == serverGrp->id())
                        groupIsSubscribed = true;
                }
                dbGrp->setSubscribed(groupIsSubscribed);
                dbGrp->commitChanges();
            }
        }
        Q_ASSERT(dbSub);
        QCoreApplication::processEvents();
    }
    // Update group lists
    foreach(ForumSubscription *serverSub, subs) {
        foreach(ForumGroup *serverGrp, serverSub->groups()) {
            Q_ASSERT(serverGrp->subscription()->parser() >= 0 || serverGrp->id().length() > 0);
            ForumSubscription *dbSub = fdb.getSubscription(serverGrp->subscription()->parser());
            Q_ASSERT(dbSub);

            ForumGroup *dbGroup = fdb.getGroup(dbSub, serverGrp->id());
            if(!dbGroup) { // Group doesn't exist yet
                qDebug() << Q_FUNC_INFO << "Group " << serverGrp->toString() << " not in db -  must add it!";
                ForumGroup *newGroup = new ForumGroup(dbSub, false);
                serverGrp->setName("?");
                serverGrp->setChangeset(-1); // Force update of group contents
                serverGrp->markToBeUpdated();
                serverGrp->setSubscribed(true);
                newGroup->copyFrom(serverGrp);
                newGroup->setChangeset(-2); // .. by setting changesets different
                fdb.addGroup(newGroup);
                dbGroup = newGroup;
            }

            Q_ASSERT(dbGroup);

            qDebug() << Q_FUNC_INFO << "Changesets for " << dbGroup->toString() << ": db " << dbGroup->changeset() << " server " << serverGrp->changeset();
            if(dbGroup->changeset() != serverGrp->changeset()) {
                qDebug() << "Adding group to download queue and setting changeset " << dbGroup->toString();
                dbGroup->setChangeset(serverGrp->changeset());
                groupsToDownload.append(dbGroup);
            }
            dbGroup->commitChanges();
            QCoreApplication::processEvents();
        }
    }

    processGroups();
}

// Sends the next group in groupsToUpload and download status for
// next in groupsToDownload
void SyncMaster::processGroups() {
    qDebug( ) << Q_FUNC_INFO;
    if(canceled) return;
    emit syncProgress((float) (maxGroupCount - groupsToUpload.size() - groupsToDownload.size()) / (float) (maxGroupCount+1));
    if (groupsToUpload.isEmpty() && groupsToDownload.isEmpty()) {
        emit syncFinished(true, QString::null);
        return;
    }
    // Do the uploading
    if(!groupsToUpload.isEmpty()) {
        qDebug() << Q_FUNC_INFO << groupsToUpload.size() << "groups to upload";
        foreach (ForumGroup* g, groupsToUpload) {
            qDebug() << Q_FUNC_INFO << g->toString();
        }
        ForumGroup *g = groupsToUpload.takeFirst();
        qDebug() << Q_FUNC_INFO << g;
        qDebug() << Q_FUNC_INFO << g->toString();
        g->setChangeset(rand());
        qDebug() << Q_FUNC_INFO << "Group " << g->toString() << "new changeset: " << g->changeset();
        foreach(ForumThread *thread, g->values()) {
            Q_ASSERT(thread);
            qDebug() << Q_FUNC_INFO << "appending thread " << thread->toString() << thread->values().size();
            messagesToUpload.append(thread->values());
        }
        connect(&protocol, SIGNAL(sendThreadDataFinished(bool, QString)), this, SLOT(sendThreadDataFinished(bool, QString)));
        protocol.sendThreadData(g, messagesToUpload);
        g->commitChanges();
        messagesToUpload.clear();
    }
    if(!groupsToDownload.isEmpty()) {
        Q_ASSERT(!groupBeingDownloaded);
        groupBeingDownloaded = groupsToDownload.takeFirst();
        qDebug() << Q_FUNC_INFO << " will now D/L group " << groupBeingDownloaded->toString();
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

void SyncMaster::serverThreadData(ForumThread *tempThread) { // Thread is temporary object!
    // qDebug() << Q_FUNC_INFO << thread->toString();
    if(canceled) return;
    if (tempThread->isSane()) {
        ForumThread *dbThread = fdb.getThread(tempThread->group()->subscription()->parser(), tempThread->group()->id(),
                                              tempThread->id());
        if (dbThread) { // Thread already found, merge it
            dbThread->setChangeset(tempThread->changeset());
        } else { // thread hasn't been found yet!
            ForumGroup *dbGroup = fdb.getGroup(fdb.getSubscription(tempThread->group()->subscription()->parser()), tempThread->group()->id());
            Q_ASSERT(dbGroup);
            Q_ASSERT(!dbGroup->isTemp());
            ForumThread *newThread = new ForumThread(dbGroup, false);
            newThread->copyFrom(tempThread);
            dbGroup->addThread(newThread);
            dbThread = newThread;
            // Make sure group will be updated
            dbGroup->markToBeUpdated();
            dbGroup->commitChanges();
        }
        dbThread->commitChanges();
    } else {
        qDebug() << "Got invalid thread!" << tempThread->toString();
        Q_ASSERT(false);
    }
}

void SyncMaster::serverMessageData(ForumMessage *tempMessage) { // Temporary object!
    qDebug() << Q_FUNC_INFO << tempMessage->toString();
    if(canceled) return;
    if (tempMessage->isSane()) {
        ForumMessage *dbMessage = fdb.getMessage(tempMessage->thread()->group()->subscription()->parser(),
                                                 tempMessage->thread()->group()->id(), tempMessage->thread()->id(), tempMessage->id());
        if (dbMessage) { // Message already found, merge it
            dbMessage->setRead(tempMessage->isRead());
        } else { // message hasn't been found yet!
            ForumThread *dbThread = fdb.getThread(tempMessage->thread()->group()->subscription()->parser(),
                                                  tempMessage->thread()->group()->id(),
                                                  tempMessage->thread()->id());
            Q_ASSERT(dbThread);
            Q_ASSERT(!dbThread->isTemp());
            ForumMessage *newMessage = new ForumMessage(dbThread, false);
            newMessage->copyFrom(tempMessage);
            newMessage->setRead(true, false);
            dbThread->addMessage(newMessage);
            dbThread->setLastPage(0); // Mark as 0 to force update of full thread
        }
    } else {
        qDebug() << Q_FUNC_INFO << "Got invalid message!" << tempMessage->toString();
        Q_ASSERT(false);
    }
    QCoreApplication::processEvents();
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
