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
	fdb = fd;
}

ParserEngine::~ParserEngine() {
}

void ParserEngine::setParser(ForumParser &fp) {
	parser = fp;
}

void ParserEngine::setSubscription(ForumSubscription &fs) {
	subscription = fs;
}
void ParserEngine::updateGroupList() {
	if (!sessionInitialized)
		session.initialize(parser, subscription);
	session.listGroups();
}

void ParserEngine::listGroupsFinished(QList<ForumGroup> groups) {
	qDebug() << "Engine rx groups " << groups.size();
	QList<ForumGroup> dbgroups = fdb->listGroups(subscription.parser);
	QList<QString> changedGroups;
	// Diff the group list
	bool groupsChanged = false;
	for (int g = 0; g < groups.size(); g++) {
		bool foundInDb = false;

		for (int d = 0; d < dbgroups.size(); d++) {
			if (dbgroups[d].id == groups[g].id) {
				foundInDb = true;
				if (dbgroups[d].lastchange != groups[g].lastchange) {
					changedGroups.append(groups[g].id);
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

	if (groupsChanged) {
		emit(groupListChanged(parser.id));
	}
}
