#include "syncmaster.h"

SyncMaster::SyncMaster(QObject *parent, ForumDatabase &fd,
		SiilihaiProtocol &prot) :
	QObject(parent), fdb(fd), protocol(prot) {
	/*
	connect(&protocol, SIGNAL(serverGroupStatus(QList<ForumGroup*>)), this,
			SLOT(serverGroupStatus(QList<ForumGroup*>)));
	connect(&protocol, SIGNAL(serverThreadData(ForumThread*)), this,
			SLOT(serverThreadData(ForumThread*)));
	connect(&protocol, SIGNAL(serverMessageData(ForumMessage*)), this,
			SLOT(serverMessageData(ForumMessage*)));
			*/
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
		{
			foreach(ForumGroup *grp, fdb.listGroups(fsub))
				{
					groupsToUpload.append(grp);
				}
		}
	processGroups();
}

void SyncMaster::serverGroupStatus(QList<ForumGroup*> grps) {
	qDebug() << Q_FUNC_INFO << grps.size();

	foreach(ForumGroup *grp, grps) {
		if(grp->subscription()->parser() <= 0 || grp->id().length() == 0) {
			qDebug() << "Insane group, skipping";
			return;
		}

		ForumGroup *dbGroup = fdb.getGroup(grp->subscription()->parser(), grp->id());
		if(!dbGroup) {
			qDebug() << "Group not in db, skipping";
			return;
		}
		qDebug() << "changesets: db " << dbGroup->changeset() << " server " << grp->changeset();
		if(dbGroup->changeset() > grp->changeset()) {
			qDebug() << "DB has newer changeset - uploading";
			groupsToUpload.append(dbGroup);
		} else if(dbGroup->changeset() < grp->changeset()) {
			qDebug() << "Server has newer changeset - downloading";
			groupsToDownload.append(dbGroup);
		} else {
			qDebug() << "Group is up to date";
		}
	}

	processGroups();
}

void SyncMaster::processGroups() {
	qDebug( ) << Q_FUNC_INFO;
	/*
	 if (!groupsToUpload.isEmpty()) {
	 ForumGroup g = groupsToUpload.first();
	 QList<ForumMessage> messages;
	 QList<ForumThread> threads = fdb.listThreads(g);
	 foreach(ForumThread thread, threads)
	 {
	 messages.append(fdb.listMessages(thread));
	 }
	 protocol.sendThreadData(messages);
	 }*/
	// groupsToUpload.clear();
	// groupsToDownload.clear();
	if (groupsToUpload.isEmpty() && groupsToDownload.isEmpty()) {
		QList<ForumSubscription*> fsubs = fdb.listSubscriptions();
		foreach(ForumSubscription *fsub, fsubs)
			{
				forumsToUpload.insert(fsub);
			}
	}
	if (!groupsToUpload.isEmpty()) {
		uploadNextGroup(true);
		return;
	}
	if (!groupsToDownload.isEmpty())
		downloadNextGroup(true);
}

void SyncMaster::downloadNextGroup(bool success) {
	qDebug() << Q_FUNC_INFO;
	if (!success) {
		qDebug() << Q_FUNC_INFO << "Failed!";
		emit
		syncFinished(false);
		return;
	}
	ForumGroup *gtodl = groupsToDownload.takeFirst();
	qDebug() << "Going to dl group " << gtodl->toString();
	connect(&protocol, SIGNAL(sendThreadDataFinished(bool)), this,
			SLOT(sendThreadDataFinished(bool)));
	protocol.getThreadData(gtodl);
}

void SyncMaster::uploadNextGroup(bool success) {
	qDebug() << Q_FUNC_INFO;
	disconnect(&protocol, SIGNAL(subscribeGroupsFinished(bool)), this,
			SLOT(uploadNextGroup(bool)));
	disconnect(&protocol, SIGNAL(sendThreadDataFinished(bool)), this,
			SLOT(sendThreadDataFinished(bool)));
	if (!success) {
		qDebug() << Q_FUNC_INFO << "Failed!";
		emit
		syncFinished(false);
		return;
	}
	// Check for messages to upload
	if (!messagesToUpload.isEmpty()) {
		connect(&protocol, SIGNAL(sendThreadDataFinished(bool)), this,
				SLOT(sendThreadDataFinished(bool)));
		protocol.sendThreadData(messagesToUpload);
		return;
	}
	// Update next forum
	if (!forumsToUpload.isEmpty()) {
		qDebug() << Q_FUNC_INFO << "still got forums to upload - subscribing groups in next";
		ForumSubscription *gtosub = *forumsToUpload.begin();
		forumsToUpload.remove(gtosub);
		QList<ForumGroup*> groups = fdb.listGroups(gtosub);
		connect(&protocol, SIGNAL(subscribeGroupsFinished(bool)), this,
				SLOT(uploadNextGroup(bool)));
		protocol.subscribeGroups(groups);
		foreach(ForumGroup *group, groups)
			{
				if (group->subscribed()) {
					QList<ForumThread*> threads = fdb.listThreads(group);
					foreach(ForumThread* thread, threads)
						{
							QList<ForumMessage*> messages = fdb.listMessages(
									thread);
							messagesToUpload.append(messages);
						}
				}
			}
	} else {
		qDebug() << Q_FUNC_INFO << "emitting sync finished";
		emit syncFinished(true);
	}
}

void SyncMaster::sendThreadDataFinished(bool success) {
	qDebug() << Q_FUNC_INFO;
	if (success) {
		messagesToUpload.clear();
		uploadNextGroup(true);
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
		if (dbMessage->isSane()) { // Thread already found, merge it
			fdb.markMessageRead(dbMessage);
		} else { // thread hasn't been found yet!
			fdb.addMessage(message);
		}
	} else {
		qDebug() << "Got invalid message!" << message->toString();
		Q_ASSERT(false);
	}

}

void SyncMaster::threadChanged(ForumThread *thread) {
qDebug() << Q_FUNC_INFO << thread->toString();

}
