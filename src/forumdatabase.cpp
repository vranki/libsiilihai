#include "forumdatabase.h"

ForumDatabase::ForumDatabase(QObject *parent) :
	QObject(parent) {
}

ForumDatabase::~ForumDatabase() {
}

bool ForumDatabase::deleteForum(const int forumid) {
	Q_ASSERT(forumid > 0);

	QList<ForumGroup> groups;
	QSqlQuery query;
	query.prepare("DELETE FROM messages WHERE (forumid=?)");
	query.addBindValue(forumid);

	if (query.exec()) {
	} else {
		qDebug() << "Unable to delete messages: " << query.lastError().text();
		Q_ASSERT(false);
	}

	query.prepare("DELETE FROM threads WHERE (forumid=?)");
	query.addBindValue(forumid);

	if (query.exec()) {
	} else {
		qDebug() << "Unable to delete threads: " << query.lastError().text();
		Q_ASSERT(false);
	}
	query.prepare("DELETE FROM groups WHERE (forumid=?)");
	query.addBindValue(forumid);

	if (query.exec()) {
	} else {
		qDebug() << "Unable to delete groups: " << query.lastError().text();
		Q_ASSERT(false);
	}
	query.prepare("DELETE FROM forums WHERE (parser=?)");
	query.addBindValue(forumid);

	if (query.exec()) {
	} else {
		qDebug() << "Unable to delete forum: " << query.lastError().text();
		Q_ASSERT(false);
	}

	return true;
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

QList<ForumSubscription> ForumDatabase::listSubscriptions() {
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
		qDebug() << "Error listing subscriptions!";
	}
	return subscriptions;
}

void ForumDatabase::resetDatabase() {
	QSqlQuery query;
	query.exec("DROP TABLE forums");
	query.exec("DROP TABLE groups");
	query.exec("DROP TABLE threads");
	query.exec("DROP TABLE messages");
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
			"changeset INTEGER, "
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
			"changeset INTEGER, "
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

QList<ForumGroup> ForumDatabase::listGroups(const int parser) {
	QList<ForumGroup> groups;
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
			g.changeset = query.value(5).toInt();
			groups.append(g);
		}
	} else {
		qDebug() << "Unable to list parsers: " << query.lastError().text();
	}

	return groups;
}

ForumGroup ForumDatabase::getGroup(const int forum, QString id) {
	ForumGroup g;
	QSqlQuery query;
	query.prepare("SELECT * FROM groups WHERE forumid=? AND groupid=?");
	query.addBindValue(forum);
	query.addBindValue(id);

	if (query.exec()) { // @todo this is copypaste
		while (query.next()) {
			g.parser = query.value(0).toInt();
			g.id = query.value(1).toString();
			g.name = query.value(2).toString();
			g.lastchange = query.value(3).toString();
			g.subscribed = query.value(4).toBool();
			g.changeset = query.value(5).toInt();
		}
	} else {
		qDebug() << "Unable to get group: " << query.lastError().text();
	}
	return g;
}

QList<ForumThread> ForumDatabase::listThreads(const ForumGroup &group) {
	QList<ForumThread> threads;
	QSqlQuery query;
	query.prepare(
			"SELECT DISTINCT * FROM threads WHERE forumid=? AND groupid=? ORDER BY ordernum");
	query.addBindValue(group.parser);
	query.addBindValue(group.id);

	if (query.exec()) {
		while (query.next()) {
			ForumThread t;
			t.forumid = query.value(0).toInt();
			t.groupid = query.value(1).toString();
			t.id = query.value(2).toString();
			t.ordernum = query.value(3).toInt();
			t.name = query.value(4).toString();
			t.lastchange = query.value(5).toString();
			t.changeset = query.value(6).toInt();
			threads.append(t);
		}
	} else {
		qDebug() << "Unable to list threads: " << query.lastError().text();
	}

	return threads;
}

ForumThread ForumDatabase::getThread(const int forum, QString groupid,
		QString threadid) {
	QSqlQuery query;
	query.prepare(
			"SELECT * FROM threads WHERE forumid=? AND groupid=? AND threadid=?");
	query.addBindValue(forum);
	query.addBindValue(groupid);
	query.addBindValue(threadid);

	if (query.exec()) {
		while (query.next()) {
			ForumThread t;
			t.forumid = query.value(0).toInt();
			t.groupid = query.value(1).toString();
			t.id = query.value(2).toString();
			t.ordernum = query.value(3).toInt();
			t.name = query.value(4).toString();
			t.lastchange = query.value(5).toString();
			t.changeset = query.value(6).toInt();
			return t;
		}
	} else {
		qDebug() << "Unable to get thread: " << query.lastError().text();
	}

	return ForumThread();
}

QList<ForumMessage> ForumDatabase::listMessages(const ForumThread &thread) {
	QList<ForumMessage> messages;
	QSqlQuery query;
	query.prepare(
			"SELECT DISTINCT * FROM messages WHERE forumid=? AND groupid=? AND threadid=? ORDER BY ordernum");
	query.addBindValue(thread.forumid);
	query.addBindValue(thread.groupid);
	query.addBindValue(thread.id);
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
	query.prepare(
			"INSERT INTO groups(forumid, groupid, name, lastchange, subscribed, changeset) VALUES (?, ?, ?, ?, ?, ?)");
	query.addBindValue(grp.parser);
	query.addBindValue(grp.id);
	query.addBindValue(grp.name);
	query.addBindValue(grp.lastchange);
	query.addBindValue(grp.subscribed);
	query.addBindValue(grp.changeset);
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
	query.prepare(
			"SELECT * FROM threads WHERE forumid=? AND groupid=? AND threadid=?");
	query.addBindValue(thread.forumid);
	query.addBindValue(thread.groupid);
	query.addBindValue(thread.id);

	query.exec();
	if (query.next()) {
		qDebug() << "Trying to add duplicate thread! " << thread.toString();
		return false;
	}

	query.prepare(
			"INSERT INTO threads(forumid, groupid, threadid, name, ordernum, lastchange, changeset) VALUES (?, ?, ?, ?, ?, ?, ?)");
	query.addBindValue(thread.forumid);
	query.addBindValue(thread.groupid);
	query.addBindValue(thread.id);
	query.addBindValue(thread.name);
	query.addBindValue(thread.ordernum);
	query.addBindValue(thread.lastchange);
	query.addBindValue(thread.changeset);
	if (!query.exec()) {
		qDebug() << "Adding thread " << thread.toString() << " failed: "
				<< query.lastError().text();
		return false;
	}
	qDebug() << "Thread " << thread.toString() << " stored";
	return true;
}

bool ForumDatabase::updateThread(const ForumThread &thread) {
	Q_ASSERT(thread.isSane());
	QSqlQuery query;
	query.prepare(
			"UPDATE threads SET name=?, ordernum=?, lastchange=?, changeset=? WHERE(forumid=? AND groupid=? AND threadid=?)");
	query.addBindValue(thread.name);
	query.addBindValue(thread.ordernum);
	query.addBindValue(thread.lastchange);
	query.addBindValue(thread.changeset);
	query.addBindValue(thread.forumid);
	query.addBindValue(thread.groupid);
	query.addBindValue(thread.id);
	if (!query.exec()) {
		qDebug() << "Updating thread " << thread.toString() << " failed: "
				<< query.lastError().text();
		return false;
	}
	qDebug() << "Thread " << thread.toString() << " updated";
	return true;
}

bool ForumDatabase::addMessage(const ForumMessage &message) {
	Q_ASSERT(message.isSane());
	QSqlQuery query;
	query.prepare(
			"SELECT * FROM messages WHERE forumid=? AND groupid=? AND threadid=? AND messageid=?");
	query.addBindValue(message.forumid);
	query.addBindValue(message.groupid);
	query.addBindValue(message.threadid);
	query.addBindValue(message.id);
	query.exec();
	if (query.next()) {
		qDebug() << "Looks like you're adding duplicate message "
				<< message.toString();
		return false;
	}

	// QSqlQuery query;
	query.prepare("INSERT INTO messages(forumid, groupid, threadid, messageid,"
		" ordernum, url, subject, author, lastchange, body, read) VALUES "
		"(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
	bindMessageValues(query, message);

	if (!query.exec()) {
		qDebug() << "Adding message " << message.toString() << " failed: "
				<< query.lastError().text();
		return false;
	}
	qDebug() << "Message " << message.toString() << " stored";
	return true;
}

ForumMessage ForumDatabase::getMessage(const int forum, QString groupid,
		QString threadid, QString messageid) {
	QSqlQuery query;
	query.prepare(
			"SELECT * FROM messages WHERE forumid=? AND groupid=? AND threadid=? AND messageid=?");
	query.addBindValue(forum);
	query.addBindValue(groupid);
	query.addBindValue(threadid);
	query.addBindValue(messageid);

	ForumMessage m;
	if (query.exec()) {
		while (query.next()) {
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
			return m;
		}
	} else {
		qDebug() << "Unable to get message: " << query.lastError().text();
	}
	return m;
}

void ForumDatabase::bindMessageValues(QSqlQuery &query,
		const ForumMessage &message) {
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
}

bool ForumDatabase::updateMessage(const ForumMessage &message) {
	QSqlQuery query;
	query.prepare(
			"UPDATE messages SET forumid=?, groupid=?, threadid=?, messageid=?,"
				" ordernum=?, url=?, subject=?, author=?, lastchange=?, body=?, read=? WHERE(forumid=? AND groupid=? AND threadid=? AND messageid=?)");

	bindMessageValues(query, message);
	query.addBindValue(message.forumid);
	query.addBindValue(message.groupid);
	query.addBindValue(message.threadid);
	query.addBindValue(message.id);
	if (!query.exec()) {
		qDebug() << "Updating message failed: " << query.lastError().text();
		Q_ASSERT(false);
		return false;
	}
	qDebug() << "Message " << message.toString() << " updated";
	return true;
}

bool ForumDatabase::deleteMessage(const ForumMessage &message) {
	Q_ASSERT(message.isSane());
	QSqlQuery query;
	query.prepare(
			"DELETE FROM messages WHERE (forumid=? AND groupid=? AND threadid=? AND messageid=?)");
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
	if (!thread.isSane()) {
		qDebug() << "Error: tried to delete invalid tread "
				<< thread.toString();
	}
	QSqlQuery query;
	query.prepare(
			"DELETE FROM threads WHERE (forumid=? AND groupid=? AND threadid=?)");
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
	query.prepare(
			"UPDATE groups SET name=?, lastchange=?, subscribed=?, changeset=? WHERE(forumid=? AND groupid=?)");
	query.addBindValue(grp.name);
	query.addBindValue(grp.lastchange);
	query.addBindValue(grp.subscribed);
	query.addBindValue(grp.changeset);
	query.addBindValue(grp.parser);
	query.addBindValue(grp.id);
	if (!query.exec()) {
		qDebug() << "Updating group failed: " << query.lastError().text();
		return false;
	}
	qDebug() << "Group " << grp.toString() << " updated, subscribed:"
			<< grp.subscribed;
	return true;
}

bool ForumDatabase::markMessageRead(const ForumMessage &message) {
	return markMessageRead(message, true);
}

bool ForumDatabase::markMessageRead(const ForumMessage &message, bool read) {
	qDebug() << Q_FUNC_INFO;
	if (!message.isSane())
		return false;
	QSqlQuery query;
	query.prepare(
			"UPDATE messages SET read=? WHERE(forumid=? AND groupid=? AND threadid=? AND messageid=?)");
	query.addBindValue(read);
	query.addBindValue(message.forumid);
	query.addBindValue(message.groupid);
	query.addBindValue(message.threadid);
	query.addBindValue(message.id);
	if (!query.exec()) {
		qDebug() << "Setting message read failed: " << query.lastError().text();
		return false;
	}
	return true;
}

int ForumDatabase::unreadIn(const ForumSubscription &fs) {
	QSqlQuery query;
	query.prepare(
			"SELECT count() FROM messages WHERE forumid=? AND read=\"false\"");
	query.addBindValue(fs.parser);

	query.exec();
	if (query.next()) {
		return query.value(0).toInt();
	} else {
		qDebug() << "Can't count unreads in " << fs.toString();
	}

	return -1;
}

int ForumDatabase::unreadIn(const ForumGroup &fg) {
	QSqlQuery query;
	query.prepare(
			"SELECT count() FROM messages WHERE forumid=? AND groupid=? AND read=\"false\"");
	query.addBindValue(fg.parser);
	query.addBindValue(fg.id);

	query.exec();
	if (query.next()) {
		return query.value(0).toInt();
	} else {
		qDebug() << "Can't count unreads in " << fg.toString();
	}

	return -1;
}

ForumSubscription ForumDatabase::getSubscription(int id) {
	QList<ForumSubscription> subscriptions = listSubscriptions();
	ForumSubscription fs;
	for (int i = 0; i < subscriptions.size(); i++) {
		if (subscriptions[i].parser == id) {
			fs = subscriptions[i];
		}
	}
	return fs;
}

bool ForumDatabase::markForumRead(const int forumid, bool read) {
	QList<ForumGroup> groups = listGroups(forumid);
	ForumGroup group;
	foreach(group, groups)
		{
			markGroupRead(group, read);
		}

	return true;
}

bool ForumDatabase::markGroupRead(const ForumGroup &group, bool read) {
	qDebug() << Q_FUNC_INFO << " " << group.toString() << ", " << read;
	QSqlQuery query;
	query.prepare("UPDATE messages SET read=? WHERE(forumid=? AND groupid=?)");
	query.addBindValue(read);
	query.addBindValue(group.parser);
	query.addBindValue(group.id);
	if (!query.exec()) {
		qDebug() << "Setting group read failed: " << query.lastError().text();
		return false;
	}
	return true;
}

int ForumDatabase::schemaVersion() {
	return 1;
}
