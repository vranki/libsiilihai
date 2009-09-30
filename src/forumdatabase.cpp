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

bool ForumDatabase::addForum(const ForumSubscription &fs) {
	QSqlQuery query;
	query.prepare("INSERT INTO forums("
		"parser, name, username, password, latest_threads, latest_messages"
		") VALUES (?, ?, ?, ?, ?, ?)");
	query.addBindValue(QString().number(fs.parser));
	query.addBindValue(fs.name);
	if (fs.username.isNull()) {
		query.addBindValue(QString(""));
		query.addBindValue(QString(""));
	} else {
		query.addBindValue(fs.username);
		query.addBindValue(fs.password);
	}
	query.addBindValue(QString().number(fs.latest_threads));
	query.addBindValue(QString().number(fs.latest_messages));

	if (!query.exec()) {
		qDebug() << "Adding forum failed: " << query.lastError().text();
		return false;
	}
	qDebug() << "Forum added";

	return true;
}

QList <ForumSubscription> ForumDatabase::listSubscriptions() {
	QList<ForumSubscription> subscriptions;
	QSqlQuery query;
	query.prepare("SELECT * FROM forums");
	if (query.exec()) {
			while (query.next()) {
				ForumSubscription fs;
				fs.parser = query.value(0).toInt();
				fs.name = query.value(1).toString();
				fs.username = query.value(2).toString();
				fs.password = query.value(3).toString();
				fs.latest_threads = query.value(4).toInt();
				fs.latest_messages = query.value(5).toInt();
				subscriptions.append(fs);
			}
	} else {
		qDebug() << "Error listing subscrioptions!";
	}
	return subscriptions;
}

bool ForumDatabase::openDatabase() {
	QSqlQuery query;
	if (!query.exec("SELECT parser FROM forums")) {
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
		if (!query.exec("CREATE TABLE groups ("
                                "forumid INTEGER REFERENCES forums(parser), "
                                "groupid VARCHAR NOT NULL, "
                                "name VARCHAR, "
                                "lastchange VARCHAR, "
                                "subscribed BOOLEAN, "
                                "PRIMARY KEY (forumid, groupid)"
                                ")")) {
			qDebug() << "Couldn't create groups table!";
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
QList <ForumGroup> ForumDatabase::listGroups(const int parser) {
	QList <ForumGroup> groups;
	QSqlQuery query;
	query.prepare("SELECT * FROM groups WHERE forumid=?");
	query.addBindValue(parser);

	if (query.exec()) {
		while (query.next()) {
			ForumGroup g;
			g.parser = query.value(0).toInt();
			g.id = query.value(1).toString();
			g.name = query.value(2).toString();
			g.lastchange = query.value(3).toString();
			g.subscribed = query.value(4).toBool();
			groups.append(g);
		}
	} else {
		qDebug() << "Unable to list parsers: " << query.lastError().text();
	}

	return groups;
}

bool ForumDatabase::addGroup(const ForumGroup &grp) {
	if(grp.id.isNull() || grp.parser < 0) {
		qDebug() << "Error: tried to add invalid group! " << grp.toString();
	}
	QSqlQuery query;
	query.prepare("INSERT INTO groups(forumid, groupid, name, lastchange, subscribed) VALUES (?, ?, ?, ?, ?)");
	query.addBindValue(grp.parser);
	query.addBindValue(grp.id);
	query.addBindValue(grp.name);
	query.addBindValue(grp.lastchange);
	query.addBindValue(grp.subscribed);
	if (!query.exec()) {
		qDebug() << "Adding group failed: " << query.lastError().text();
		return false;
	}
	qDebug() << "Group " << grp.toString() << " stored";
	return true;
}

bool ForumDatabase::deleteGroup(const ForumGroup &grp) {
	if(grp.id.isNull() || grp.parser < 0) {
		qDebug() << "Error: tried to delete invalid group! " << grp.toString();
	}
	QSqlQuery query;
	query.prepare("DELETE FROM groups WHERE (forumid=? AND groupid=?)");
	query.addBindValue(grp.parser);
	query.addBindValue(grp.id);
	if (!query.exec()) {
		qDebug() << "Deleting group failed: " << query.lastError().text();
		return false;
	}
	qDebug() << "Group " << grp.toString() << " deleted";
	return true;
}
