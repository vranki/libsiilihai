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
	Q_ASSERT(fs.isSane());
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

QList <ForumThread> ForumDatabase::listThreads(const int parser, QString group) {
	QList <ForumThread> threads;
	QSqlQuery query;
	query.prepare("SELECT * FROM threads WHERE forumid=? AND groupid=?");
	query.addBindValue(parser);
	query.addBindValue(group);

	if (query.exec()) {
		while (query.next()) {
			ForumThread t;
			t.forumid = query.value(0).toInt();
			t.groupid = query.value(1).toString();
			t.id = query.value(2).toString();
			t.ordernum = query.value(3).toInt();
			t.name = query.value(4).toString();
			t.lastchange = query.value(5).toString();
			threads.append(t);
		}
	} else {
		qDebug() << "Unable to list threads: " << query.lastError().text();
	}

	return threads;
}

QList <ForumMessage> ForumDatabase::listMessages(const int parser, QString group, QString thread) {
	QList <ForumMessage> messages;
	QSqlQuery query;
	query.prepare("SELECT * FROM messages WHERE forumid=? AND groupid=? AND threadid=?");
	query.addBindValue(parser);
	query.addBindValue(group);
	query.addBindValue(thread);
	if (query.exec()) {
		while (query.next()) {
			ForumMessage m;
			m.forumid = query.value(0).toInt();
			m.groupid = query.value(1).toString();
			m.threadid = query.value(2).toString();
			m.id = query.value(3).toString();
			m.ordernum = query.value(4).toInt();
			m.url = query.value(5).toString();
			m.subject = query.value(6).toString();
			m.author = query.value(7).toString();
			m.lastchange = query.value(8).toString();
			m.body = query.value(9).toString();
			m.read = query.value(10).toBool();
			messages.append(m);
		}
	} else {
		qDebug() << "Unable to list messages: " << query.lastError().text();
	}
	return messages;
}

bool ForumDatabase::addGroup(const ForumGroup &grp) {
	Q_ASSERT(grp.isSane());

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

bool ForumDatabase::addThread(const ForumThread &thread) {
	Q_ASSERT(thread.isSane());
	QSqlQuery query;
	query.prepare("INSERT INTO threads(forumid, groupid, threadid, name, ordernum, lastchange) VALUES (?, ?, ?, ?, ?, ?)");
	query.addBindValue(thread.forumid);
	query.addBindValue(thread.groupid);
	query.addBindValue(thread.id);
	query.addBindValue(thread.name);
	query.addBindValue(thread.ordernum);
	query.addBindValue(thread.lastchange);
	if (!query.exec()) {
		qDebug() << "Adding thread " << thread.toString() << " failed: " << query.lastError().text();
		return false;
	}
	qDebug() << "Thread " << thread.toString() << " stored";
	return true;
}

bool ForumDatabase::addMessage(const ForumMessage &message) {
	Q_ASSERT(message.isSane());
	QSqlQuery query;
	query.prepare("INSERT INTO messages(forumid, groupid, threadid, messageid,"\
			" ordernum, url, subject, author, lastchange, body, read) VALUES "\
			"(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
	query.addBindValue(message.forumid);
	query.addBindValue(message.groupid);
	query.addBindValue(message.threadid);
	query.addBindValue(message.id);
	query.addBindValue(message.ordernum);
	query.addBindValue(message.url);
	query.addBindValue(message.subject);
	query.addBindValue(message.author);
	query.addBindValue(message.lastchange);
	query.addBindValue(message.body);
	query.addBindValue(message.read);

	if (!query.exec()) {
		qDebug() << "Adding message " << message.toString() << " failed: " << query.lastError().text();
		return false;
	}
	qDebug() << "Message " << message.toString() << " stored";
	return true;
}

bool ForumDatabase::deleteMessage(const ForumMessage &message) {
	Q_ASSERT(message.isSane());
	QSqlQuery query;
	query.prepare("DELETE FROM messages WHERE (forumid=? AND groupid=? AND threadid=? AND messageid=?)");
	query.addBindValue(message.forumid);
	query.addBindValue(message.groupid);
	query.addBindValue(message.threadid);
	query.addBindValue(message.id);
	if (!query.exec()) {
		qDebug() << "Deleting message failed: " << query.lastError().text();
		return false;
	}
	qDebug() << "Message " << message.toString() << " deleted";
	return true;
}

bool ForumDatabase::deleteGroup(const ForumGroup &grp) {
	Q_ASSERT(grp.isSane());

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

bool ForumDatabase::deleteThread(const ForumThread &thread) {
	if(!thread.isSane()) {
		qDebug() << "Error: tried to delete invalid tread " << thread.toString();
	}
	QSqlQuery query;
	query.prepare("DELETE FROM threads WHERE (forumid=? AND groupid=? AND threadid=?)");
	query.addBindValue(thread.forumid);
	query.addBindValue(thread.groupid);
	query.addBindValue(thread.id);
	if (!query.exec()) {
		qDebug() << "Deleting thread failed: " << query.lastError().text();
		return false;
	}
	qDebug() << "Thread " << thread.toString() << " deleted";
	return true;
}

bool ForumDatabase::updateGroup(const ForumGroup &grp) {
	Q_ASSERT(grp.isSane());
	QSqlQuery query;
	query.prepare("UPDATE groups SET name=?, lastchange=?, subscribed=? WHERE(forumid=? AND groupid=?)");
	query.addBindValue(grp.name);
	query.addBindValue(grp.lastchange);
	query.addBindValue(grp.subscribed);
	query.addBindValue(grp.parser);
	query.addBindValue(grp.id);
	if (!query.exec()) {
		qDebug() << "Updating group failed: " << query.lastError().text();
		return false;
	}
	qDebug() << "Group " << grp.toString() << " updated";
	return true;
}
