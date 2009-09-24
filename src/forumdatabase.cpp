/*
 * forumdatabase.cpp
 *
 *  Created on: Sep 23, 2009
 *      Author: vranki
 */

#include "forumdatabase.h"

ForumDatabase::ForumDatabase(QObject *parent) :
	QObject(parent) {
}

ForumDatabase::~ForumDatabase() {
}

bool ForumDatabase::addForum(int parser, QString name, QString username, QString password,
		int latest_threads, int latest_messages) {
	QSqlQuery query;
	query.prepare(
			"INSERT INTO forums("
				"id, name, username, password, latest_threads, latest_messages"
				") VALUES (?, ?, ?, ?, ?, ?, ?)");
	query.addBindValue(parser);
	query.addBindValue(name);
	query.addBindValue(username);
	query.addBindValue(password);
	query.addBindValue(latest_threads);
	query.addBindValue(latest_messages);

	if (!query.exec()) {
		qDebug() << "Adding forum failed: " << query.lastError().text();
		return false;
	}
	return true;
}

bool ForumDatabase::openDatabase() {
	QSqlQuery query;
	if (!query.exec("SELECT forumid FROM forums")) {
		qDebug("DB doesn't exist, creating..");
		if (!query.exec("CREATE TABLE forums ("
			"parser INTEGER PRIMARY KEY REFERENCES parsers(id), "
			"name VARCHAR, "
			"username VARCHAR, "
			"password VARCHAR, "
			"latest_threads INTEGER, "
			"latest_messages INTEGER"
			")")) {
			qDebug() << "Couldn't create forums table!";
			return false;
		}
		if (!query.exec("CREATE TABLE threads ("
			"forumid INTEGER REFERENCES forums(forumid), "
			"groupid VARCHAR REFERENCES groups(groupid), "
			"threadid VARCHAR NOT NULL, "
			"ordernum INTEGER, "
			"name VARCHAR, "
			"url VARCHAR, "
			"lastchange VARCHAR, "
			"PRIMARY KEY (forumid, groupid, threadid)"
			")")) {
			qDebug() << "Couldn't create threads table!";
			return false;
		}
		if (!query.exec("CREATE TABLE messages ("
			"forumid INTEGER REFERENCES forums(forumid), "
			"groupid VARCHAR REFERENCES groups(groupid), "
			"threadid VARCHAR REFERENCES threads(threadid), "
			"messageid VARCHAR NOT NULL, "
			"ordernum INTEGER, "
			"url VARCHAR, "
			"subject VARCHAR, "
			"author VARCHAR, "
			"lastchange VARCHAR, "
			"body VARCHAR, "
			"read BOOLEAN, "
			"PRIMARY KEY (forumid, groupid, threadid, messageid)"
			")")) {
			qDebug() << "Couldn't create messages table!";
			return false;
		}
	}
	return true;
}
