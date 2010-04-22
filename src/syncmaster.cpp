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
    connect(&protocol, SIGNAL(getThreadDataFinished(bool)), this, SLOT(getThreadDataFinished(bool)));
}

SyncMaster::~SyncMaster() {
}

void SyncMaster::startSync() {
    protocol.getSyncSummary();
}

void SyncMaster::endSync() {
    qDebug( ) << Q_FUNC_INFO;
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

    if (groupsToUpload.isEmpty() && groupsToDownload.isEmpty()) {
        emit syncFinished(true);
        return;
    }
    // Do the uploading
    if(!groupsToUpload.isEmpty()) {
        ForumGroup *g = groupsToUpload.takeFirst();
        foreach(ForumThread *thread, fdb.listThreads(g))
        {
            messagesToUpload.append(fdb.listMessages(thread));
        }
        connect(&protocol, SIGNAL(sendThreadDataFinished(bool)), this, SLOT(sendThreadDataFinished(bool)));
        protocol.sendThreadData(messagesToUpload);
        messagesToUpload.clear();
    }
    if(!groupsToDownload.isEmpty()) {
        ForumGroup *g = groupsToDownload.takeFirst();
        qDebug() << Q_FUNC_INFO << " will now D/L group " << g->toString();
        protocol.getThreadData(g);
    }
}

void SyncMaster::sendThreadDataFinished(bool success) {
    qDebug() << Q_FUNC_INFO << success;
    disconnect(&protocol, SIGNAL(sendThreadDataFinished(bool)), this, SLOT(sendThreadDataFinished(bool)));
    if (success) {
        processGroups();
    } else {
        qDebug() << Q_FUNC_INFO << "Failed!";
        emit syncFinished(false);
    }
}

void SyncMaster::serverThreadData(ForumThread *thread) {
    qDebug() << Q_FUNC_INFO << thread->toString();
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

void SyncMaster::getThreadDataFinished(bool success){
    qDebug() << Q_FUNC_INFO << success;
    if(success) {
        processGroups();
    } else {
        emit syncFinished(false);
    }
}

void SyncMaster::threadChanged(ForumThread *thread) {
    qDebug() << Q_FUNC_INFO << thread->toString();
}

