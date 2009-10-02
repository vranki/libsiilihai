/*
 * parserengine.cpp
 *
 *  Created on: Sep 18, 2009
 *      Author: vranki
 */

#include "parserengine.h"

ParserEngine::ParserEngine(ForumDatabase *fd, QObject *parent) :
	QObject(parent), session(this) {
	sessionInitialized = false;
	connect(&session, SIGNAL(listGroupsFinished(QList<ForumGroup>)), this,
			SLOT(listGroupsFinished(QList<ForumGroup>)));
	connect(&session, SIGNAL(listThreadsFinished(QList<ForumThread>, ForumGroup)), this,
			SLOT(listThreadsFinished(QList<ForumThread>, ForumGroup)));
	connect(&session, SIGNAL(listMessagesFinished(QList<ForumMessage>, ForumThread)), this,
			SLOT(listMessagesFinished(QList<ForumMessage>, ForumThread)));
	fdb = fd;
	updateAll = false;
}

ParserEngine::~ParserEngine() {
}

void ParserEngine::setParser(ForumParser &fp) {
	parser = fp;
}

void ParserEngine::setSubscription(ForumSubscription &fs) {
	subscription = fs;
}

void ParserEngine::updateForum() {
	emit statusChanged(parser.id, true);
	updateAll = true;
	if (!sessionInitialized)
		session.initialize(parser, subscription);
	session.listGroups();
}

void ParserEngine::updateGroupList() {
	emit statusChanged(parser.id, true);
	updateAll = false;
	if (!sessionInitialized)
		session.initialize(parser, subscription);
	session.listGroups();
}

void ParserEngine::updateNextChangedGroup() {
	if(groupsToUpdateQueue.size() > 0) {
		session.updateGroup(groupsToUpdateQueue.dequeue());
	} else {
		qDebug() << "No more changed groups - end of update.";
		updateAll = false;
		Q_ASSERT(groupsToUpdateQueue.size()==0 &&
				threadsToUpdateQueue.size()==0);
		emit forumUpdated(parser.id);
		emit statusChanged(parser.id, false);
	}
}

void ParserEngine::updateNextChangedThread() {
	if(threadsToUpdateQueue.size() > 0) {
		session.updateThread(threadsToUpdateQueue.dequeue());
	} else {
		updateNextChangedGroup();
	}
}

void ParserEngine::listGroupsFinished(QList<ForumGroup> groups) {
	qDebug() << "ParserEngine::listGroupsFinished rx groups " << groups.size();
	QList<ForumGroup> dbgroups = fdb->listGroups(subscription.parser);
	groupsToUpdateQueue.clear();
	// Diff the group list
	bool groupsChanged = false;
	for (int g = 0; g < groups.size(); g++) {
		bool foundInDb = false;

		for (int d = 0; d < dbgroups.size(); d++) {
			if (dbgroups[d].id == groups[g].id) {
				foundInDb = true;
				// @todo testi:
				if (dbgroups[d].id == "4" ||dbgroups[d].lastchange != groups[g].lastchange) {
					groupsToUpdateQueue.enqueue(groups[g]);
					qDebug() << "Group " << dbgroups[d].toString()
							<< " has been changed, adding to list";
				}
			}
		}
		if (!foundInDb) {
			groupsChanged = true;
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
	if (updateAll && groupsToUpdateQueue.size() > 0) {
		updateNextChangedGroup();
	}
	if (groupsChanged) {
		emit(groupListChanged(parser.id));
	}
	emit statusChanged(parser.id, false);
}

void ParserEngine::listThreadsFinished(QList<ForumThread> threads, ForumGroup group) {
	qDebug() << "Engine rx threads " << threads.size() << " in group " << group.toString();
	QList<ForumThread> dbthreads = fdb->listThreads(subscription.parser, group.id);
	threadsToUpdateQueue.clear();
	// Diff the group list
	bool threadsChanged = false;
	for (int t = 0; t < threads.size(); t++) {
		bool foundInDb = false;

		for (int d = 0; d < dbthreads.size(); d++) {
			if (dbthreads[d].id == threads[t].id) {
				foundInDb = true;
				// @todo testi:
				if (dbthreads[d].id == "203" || dbthreads[d].lastchange != threads[t].lastchange) {
					threadsToUpdateQueue.enqueue(threads[t]);
					qDebug() << "Thread " << dbthreads[d].toString()
							<< " has been changed, adding to list";
				}
			}
		}
		if (!foundInDb) {
			threadsChanged = true;
			fdb->addThread(threads[t]);
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
	if (updateAll && threadsToUpdateQueue.size() > 0) {
		updateNextChangedThread();
	}
	/*
	if (threadsChanged) {
		emit(threadListChanged(parser.id));
	}*/
}

void ParserEngine::listMessagesFinished(QList<ForumMessage> messages, ForumThread thread) {
	qDebug() << "PE: listmessagesFinished " << messages.size();
	QList<ForumMessage> dbmessages = fdb->listMessages(subscription.parser, thread.groupid, thread.id);
	// Diff the group list
	bool messagesChanged = false;
	for (int t = 0; t < messages.size(); t++) {
		bool foundInDb = false;

		for (int d = 0; d < dbmessages.size(); d++) {
			if (dbmessages[d].id == messages[t].id) {
				foundInDb = true;
				// @todo testi:
				if (dbmessages[d].id == "1096" || dbmessages[d].lastchange != messages[t].lastchange) {
					qDebug() << "Message " << dbmessages[d].toString()
							<< " has been changed.";
					// @todo update in db
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
	if (updateAll) {
		updateNextChangedThread();
	}
}
