#include "parserengine.h"

ParserEngine::ParserEngine(ForumDatabase *fd, QObject *parent) :
	QObject(parent), session(this) {
	sessionInitialized = false;
	connect(&session, SIGNAL(listGroupsFinished(QList<ForumGroup>)), this,
			SLOT(listGroupsFinished(QList<ForumGroup>)));
	connect(&session,
			SIGNAL(listThreadsFinished(QList<ForumThread>, ForumGroup)), this,
			SLOT(listThreadsFinished(QList<ForumThread>, ForumGroup)));
	connect(&session,
			SIGNAL(listMessagesFinished(QList<ForumMessage>, ForumThread)),
			this, SLOT(listMessagesFinished(QList<ForumMessage>, ForumThread)));
	connect(&session,
			SIGNAL(networkFailure(QString)),
			this, SLOT(networkFailure(QString)));
	fdb = fd;
	updateAll = false;
	forceUpdate = false;
	forumBusy = 0;
}

ParserEngine::~ParserEngine() {
}

void ParserEngine::setParser(ForumParser &fp) {
	parser = fp;
}

void ParserEngine::setSubscription(ForumSubscription &fs) {
	subscription = fs;
}

void ParserEngine::updateForum(bool force) {
	qDebug() << "updateForum() called for forum " << parser.toString();
	forceUpdate = force;
	setBusy(true);
	largestGroupsToUpdateQueue = 0;
	largestThreadsToUpdateQueue = 0;

	updateAll = true;
	if (!sessionInitialized)
		session.initialize(parser, subscription);
	session.listGroups();
	updateCurrentProgress();
}

void ParserEngine::networkFailure(QString message) {
	emit updateFailure("Updating " + subscription.name + " failed due to network error:\n\n" + message);
	cancelOperation();
}

void ParserEngine::updateGroupList() {
	setBusy(true);
	updateAll = false;
	largestGroupsToUpdateQueue = 0;
	largestThreadsToUpdateQueue = 0;
	if (!sessionInitialized)
		session.initialize(parser, subscription);
	session.listGroups();
	updateCurrentProgress();
}

void ParserEngine::updateNextChangedGroup() {
	Q_ASSERT(updateAll);

	if (groupsToUpdateQueue.size() > 0) {
		session.listThreads(groupsToUpdateQueue.dequeue());
	} else {
		qDebug() << "No more changed groups - end of update.";
		updateAll = false;
		Q_ASSERT(groupsToUpdateQueue.size()==0 &&
				threadsToUpdateQueue.size()==0);
		setBusy(false);
		emit
		forumUpdated(parser.id);
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

void ParserEngine::listGroupsFinished(QList<ForumGroup> groups) {
	qDebug() << Q_FUNC_INFO << " rx groups " << groups.size()
			<< " in " << parser.toString();
	QList<ForumGroup> dbgroups = fdb->listGroups(subscription.parser);

	groupsToUpdateQueue.clear();
	if (groups.size() == 0 && dbgroups.size() > 0) {
		emit updateFailure("Updating group list for " + parser.parser_name
				+ " failed. \nCheck your network connection.");
		cancelOperation();
		return;
	}
	// Diff the group list
	bool groupsChanged = false;
	for (int g = 0; g < groups.size(); g++) {
		bool foundInDb = false;

		for (int d = 0; d < dbgroups.size(); d++) {
			if (dbgroups[d].id == groups[g].id) {
				foundInDb = true;
				if ((dbgroups[d].subscribed && ((dbgroups[d].lastchange
						!= groups[g].lastchange) || forceUpdate))) {
					groupsToUpdateQueue.enqueue(groups[g]);
					qDebug() << "Group " << dbgroups[d].toString()
							<< " has been changed, adding to list";
					// Store the updated version to database
					groups[g].subscribed = true;
					fdb->updateGroup(groups[g]);
				} else {
					qDebug() << "Group" << dbgroups[d].toString()
							<< " hasn't changed or is not subscribed - not reloading.";
				}
			}
		}
		if (!foundInDb) {
			qDebug() << "Group " << groups[g].toString()
					<< " not found in db - adding.";
			groupsChanged = true;
			if (!updateAll) {
				// DON'T set lastchange when only updating group list.
				groups[g].lastchange = "UPDATE_NEEDED";
			} else {
				groupsToUpdateQueue.enqueue(groups[g]);
			}
			fdb->addGroup(groups[g]);
		}
	}

	// check for DELETED groups
	for (int d = 0; d < dbgroups.size(); d++) {
		bool groupFound = false;
		for (int g = 0; g < groups.size(); g++) {
			if (dbgroups[d].id == groups[g].id) {
				groupFound = true;
			}
		}
		if (!groupFound) {
			groupsChanged = true;
			qDebug() << "Group " << dbgroups[d].toString()
					<< " has been deleted!";
			fdb->deleteGroup(dbgroups[d]);
		}
	}
	if (updateAll) {
		updateNextChangedGroup();
	} else {
		setBusy(false);
	}
	if (groupsChanged) {
		emit(groupListChanged(parser.id));
	}
}

void ParserEngine::listThreadsFinished(QList<ForumThread> threads,
		ForumGroup group) {
	qDebug() << "Engine rx threads " << threads.size() << " in group "
			<< group.toString();
	QList<ForumThread> dbthreads = fdb->listThreads(group);
	threadsToUpdateQueue.clear();
	if (threads.size() == 0 && dbthreads.size() > 0) {
		emit updateFailure(
				"Updating thread list failed. \nCheck your network connection.");
		cancelOperation();
		return;
	}

	// Diff the group list
	bool threadsChanged = false;
	for (int t = 0; t < threads.size(); t++) {
		bool foundInDb = false;

		for (int d = 0; d < dbthreads.size(); d++) {
			if (dbthreads[d].id == threads[t].id) {
				foundInDb = true;
				if ((dbthreads[d].lastchange != threads[t].lastchange) || forceUpdate) {
					threadsToUpdateQueue.enqueue(threads[t]);
					qDebug() << "Thread " << dbthreads[d].toString()
							<< " has been changed, adding to list";
				}
			}
		}
		if (!foundInDb) {
			threadsChanged = true;
			fdb->addThread(threads[t]);
			threadsToUpdateQueue.enqueue(threads[t]);
		}
	}

	// check for DELETED threads
	for (int d = 0; d < dbthreads.size(); d++) {
		bool threadFound = false;
		for (int t = 0; t < threads.size(); t++) {
			if (dbthreads[d].id == threads[t].id) {
				threadFound = true;
			}
		}
		if (!threadFound) {
			threadsChanged = true;
			qDebug() << "Thread " << dbthreads[d].toString()
					<< " has been deleted!";
			fdb->deleteThread(dbthreads[d]);
		}
	}
	if(operatio)
	updateNextChangedThread();
}

void ParserEngine::listMessagesFinished(QList<ForumMessage> messages,
		ForumThread thread) {
	qDebug() << "PE: listmessagesFinished " << messages.size() << "new in "
			<< thread.toString();
	QList<ForumMessage> dbmessages = fdb->listMessages(thread);
	// Diff the group list
	qDebug() << "db contains " << dbmessages.size() << " msgs, got now "
			<< messages.size() << " thread " << thread.toString();
	bool messagesChanged = false;
	for (int t = 0; t < messages.size(); t++) {
		bool foundInDb = false;

		for (int d = 0; d < dbmessages.size(); d++) {
			if (dbmessages[d].id == messages[t].id) {
				// qDebug() << "msg id " << dbmessages[d].id << " & " << messages[t].id;
				foundInDb = true;
				if ((dbmessages[d].lastchange != messages[t].lastchange) || forceUpdate) {
					qDebug() << "Message " << dbmessages[d].toString()
							<< " has been changed.";
					// @todo update in db
					fdb->updateMessage(messages[t]);
				}
			}
		}
		if (!foundInDb) {
			messagesChanged = true;
			fdb->addMessage(messages[t]);
		}
	}

	// check for DELETED threads
	for (int d = 0; d < dbmessages.size(); d++) {
		bool messageFound = false;
		for (int t = 0; t < messages.size(); t++) {
			if (dbmessages[d].id == messages[t].id) {
				messageFound = true;
			}
		}
		if (!messageFound) {
			messagesChanged = true;
			qDebug() << "Message " << dbmessages[d].toString()
					<< " has been deleted!";
			fdb->deleteMessage(dbmessages[d]);
		}
	}
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

	int gsize = groupsToUpdateQueue.size();
	int tsize = threadsToUpdateQueue.size();
	if (gsize > largestGroupsToUpdateQueue)
		largestGroupsToUpdateQueue = gsize;
	if (tsize > largestThreadsToUpdateQueue)
		largestThreadsToUpdateQueue = tsize;

	if (largestGroupsToUpdateQueue > 0) {
		float lgroups = largestGroupsToUpdateQueue;
		float lthreads = largestThreadsToUpdateQueue;
		float groups = gsize;
		float threads = tsize;

		progress = groups / lgroups;
	}
	emit statusChanged(parser.id, forumBusy, progress);
}

void ParserEngine::cancelOperation() {
	updateAll = false;
	session.cancelOperation();
	groupsToUpdateQueue.clear();
	threadsToUpdateQueue.clear();
	setBusy(false);
}
