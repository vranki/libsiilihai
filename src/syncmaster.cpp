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

    connect(&protocol, SIGNAL(serverGroupStatus(QList<ForumGroup> &)), this,
            SLOT(serverGroupStatus(QList<ForumGroup> &)));
    connect(&protocol, SIGNAL(serverThreadData(ForumThread*)), this,
            SLOT(serverThreadData(ForumThread*)));
    connect(&protocol, SIGNAL(serverMessageData(ForumMessage*)), this,
            SLOT(serverMessageData(ForumMessage*)));
    connect(&protocol, SIGNAL(getThreadDataFinished(bool, QString)), this,
            SLOT(getThreadDataFinished(bool, QString)));
    canceled = true;
}

SyncMaster::~SyncMaster() {
}

void SyncMaster::startSync() {
    canceled = false;
    protocol.getSyncSummary();
}

void SyncMaster::endSync() {
    qDebug( ) << Q_FUNC_INFO;
    canceled = false;
    QList<ForumSubscription*> fsubs = fdb.listSubscriptions();

    foreach(ForumSubscription *fsub, fsubs)
        foreach(ForumGroup *grp, fdb.listGroups(fsub))
            if(grp->subscribed())
                groupsToUpload.append(grp);

    processGroups();
}

void SyncMaster::serverGroupStatus(QList<ForumGroup> &grps) {
    qDebug() << Q_FUNC_INFO << grps.size();

    foreach(ForumGroup grp, grps) {
        Q_ASSERT(grp.subscription()->parser() >= 0 || grp.id().length() > 0);

        ForumSubscription *dbSub = fdb.getSubscription(grp.subscription()->parser());
        if(!dbSub) {
            qDebug() << Q_FUNC_INFO << "Forum not in db -  must add it!";
            ForumSubscription newSub;
            newSub.setParser(grp.subscription()->parser());
            newSub.setAlias(grp.subscription()->alias());
            newSub.setLatestThreads(grp.subscription()->latest_threads());
            newSub.setLatestMessages(grp.subscription()->latest_messages());

            dbSub = fdb.addForum(&newSub);
        }

        Q_ASSERT(dbSub);

        ForumGroup *dbGroup = fdb.getGroup(dbSub, grp.id());
        fdb.getGroup(dbSub, grp.id());
        if(!dbGroup) {
            qDebug() << Q_FUNC_INFO << "Group " << grp.toString() << " not in db -  must add it!";
            ForumGroup group(dbSub);
            group.setName("?");
            group.setId(grp.id());
            group.setChangeset(0);
            group.setSubscribed(true);
            fdb.getGroup(dbSub, grp.id());
            dbGroup = fdb.addGroup(&group);
            fdb.getGroup(dbSub, grp.id());
        }

        Q_ASSERT(dbGroup);

        qDebug() << "changesets: db " << dbGroup->changeset() << " server " << grp.changeset();
        groupsToDownload.append(dbGroup);
        /*
        if(dbGroup->changeset() > grp->changeset()) {
            qDebug() << "DB has newer changeset - uploading";
            groupsToUpload.append(dbGroup);
        } else if(dbGroup->changeset() < grp->changeset()) {
            qDebug() << "Server has newer changeset - downloading";
            groupsToDownload.append(dbGroup);
        } else {
            qDebug() << "Group is up to date";
        }
        */
    }

    processGroups();
}

// Sends the next group in groupsToUpload and download status for
// next in groupsToDownload
void SyncMaster::processGroups() {
    qDebug( ) << Q_FUNC_INFO;
    if(canceled) return;
    if (groupsToUpload.isEmpty() && groupsToDownload.isEmpty()) {
        emit syncFinished(true, QString::null);
        return;
    }
    // Do the uploading
    if(!groupsToUpload.isEmpty()) {
        ForumGroup *g = groupsToUpload.takeFirst();
        foreach(ForumThread *thread, fdb.listThreads(g))
        {
            messagesToUpload.append(fdb.listMessages(thread));
        }
        connect(&protocol, SIGNAL(sendThreadDataFinished(bool, QString)),
                this, SLOT(sendThreadDataFinished(bool, QString)));
        protocol.sendThreadData(g, messagesToUpload);
        messagesToUpload.clear();
    }
    if(!groupsToDownload.isEmpty()) {
        ForumGroup *g = groupsToDownload.takeFirst();
        qDebug() << Q_FUNC_INFO << " will now D/L group " << g->toString();
        protocol.getThreadData(g);
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
        qDebug() << Q_FUNC_INFO << "Failed!";
        emit syncFinished(false, message);
    }
}

void SyncMaster::serverThreadData(ForumThread *thread) {
    qDebug() << Q_FUNC_INFO << thread->toString();
    if(canceled) return;
    if (thread->isSane()) {
        ForumThread *dbThread = fdb.getThread(thread->group()->subscription()->parser(), thread->group()->id(),
                                              thread->id());
        if (dbThread) { // Thread already found, merge it
            dbThread->setChangeset(thread->changeset());
            fdb.updateThread(dbThread);
        } else { // thread hasn't been found yet!
            ForumGroup *dbGroup = fdb.getGroup(fdb.getSubscription(thread->group()->subscription()->parser()), thread->group()->id());
            Q_ASSERT(dbGroup);
            thread->setGroup(dbGroup);
            fdb.addThread(thread);
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
        if (dbMessage && dbMessage->isSane()) { // Message already found, merge it
            if(dbMessage->read() != message->read()) {
                dbMessage->setRead(message->read());
                fdb.updateMessage(dbMessage);
            }
        } else { // message hasn't been found yet!
            ForumThread *dbThread = fdb.getThread(message->thread()->group()->subscription()->parser(),
                                                 message->thread()->group()->id(),
                                                 message->thread()->id());
            Q_ASSERT(dbThread);
            message->setThread(dbThread);
            fdb.addMessage(message);
        }
    } else {
        qDebug() << "Got invalid message!" << message->toString();
        Q_ASSERT(false);
    }
}

void SyncMaster::getThreadDataFinished(bool success, QString message){
    qDebug() << Q_FUNC_INFO << success;
    if(canceled) return;
    if(success) {
        processGroups();
    } else {
        emit syncFinished(false, message);
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
