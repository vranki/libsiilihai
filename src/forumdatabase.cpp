#include "forumdatabase.h"

ForumDatabase::ForumDatabase(QObject *parent) :
	QObject(parent) {
}

ForumDatabase::~ForumDatabase() {
}

bool ForumDatabase::deleteForum(ForumSubscription *sub) {
	Q_ASSERT(sub);
	Q_ASSERT(subscriptions[sub->parser()]);

	QSqlQuery query;
	/*
	query.prepare("DELETE FROM messages WHERE (forumid=?)");
	query.addBindValue(sub->parser());

	if (query.exec()) {
	} else {
		qDebug() << "Unable to delete messages: " << query.lastError().text();
		Q_ASSERT(false);
	}

	query.prepare("DELETE FROM threads WHERE (forumid=?)");
	query.addBindValue(sub->parser());

	if (query.exec()) {
	} else {
		qDebug() << "Unable to delete threads: " << query.lastError().text();
		Q_ASSERT(false);
	}
	query.prepare("DELETE FROM groups WHERE (forumid=?)");
	query.addBindValue(sub->parser());

	if (query.exec()) {
	} else {
		qDebug() << "Unable to delete groups: " << query.lastError().text();
		Q_ASSERT(false);
	}
	*/
	foreach(ForumGroup* g, listGroups(sub)) {
		deleteGroup(g);
	}

	query.prepare("DELETE FROM forums WHERE (parser=?)");
	query.addBindValue(sub->parser());

	if (query.exec()) {
	} else {
		qDebug() << "Unable to delete forum" << sub->toString() << ": " << query.lastError().text();
		Q_ASSERT(false);
		return false;
	}

	subscriptions.erase(subscriptions.find(sub->parser()));
	while(!groups[sub].isEmpty()) {
		deleteGroup(*groups[sub].begin());
	}
	emit subscriptionDeleted(sub);
	qDebug() << "Subscription " << sub->toString() << " deleted";
	sub->deleteLater();
	return true;
}

ForumSubscription* ForumDatabase::addForum(ForumSubscription *fs) {
	Q_ASSERT(fs);
	Q_ASSERT(fs->isSane());
	Q_ASSERT(!subscriptions[fs->parser()]);
	QSqlQuery query;
	query.prepare("INSERT INTO forums("
		"parser, name, username, password, latest_threads, latest_messages"
		") VALUES (?, ?, ?, ?, ?, ?)");
	query.addBindValue(QString().number(fs->parser()));
	query.addBindValue(fs->name());
	if (fs->username().isNull()) {
		query.addBindValue(QString(""));
		query.addBindValue(QString(""));
	} else {
		query.addBindValue(fs->username());
		query.addBindValue(fs->password());
	}
	query.addBindValue(QString().number(fs->latest_threads()));
	query.addBindValue(QString().number(fs->latest_messages()));

	if (!query.exec()) {
		qDebug() << "Adding forum failed: " << query.lastError().text();
		return 0;
	}
	ForumSubscription *nfs = new ForumSubscription(this);
	qDebug() << "nfs=" << nfs << " fs=" << fs;
	nfs->operator=(*fs);
	qDebug() << "nfs=" << nfs;
	subscriptions[nfs->parser()] = nfs;
	emit subscriptionAdded(nfs);
	emit subscriptionFound(nfs);
	qDebug() << "Forum added: " << nfs->toString();

	return nfs;
}

QList<ForumSubscription*> ForumDatabase::listSubscriptions() {
	QSqlQuery query;
	query.prepare("SELECT * FROM forums");
	if (query.exec()) {
		while (query.next()) {
			if(!subscriptions.contains(query.value(0).toInt())) {
				ForumSubscription *fs = new ForumSubscription(this);
				fs->setParser(query.value(0).toInt());
				fs->setName(query.value(1).toString());
				fs->setUsername(query.value(2).toString());
				fs->setPassword(query.value(3).toString());
				fs->setLatestThreads(query.value(4).toInt());
				fs->setLatestMessages(query.value(5).toInt());
				subscriptions[fs->parser()] = fs;
				emit subscriptionFound(fs);
				listGroups(fs);
			}
		}
	} else {
		qDebug() << "Error listing subscriptions!";
	}
	return subscriptions.values();
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

QList <ForumGroup*> ForumDatabase::listGroups(ForumSubscription *sub) {
	QSqlQuery query;
	query.prepare("SELECT * FROM groups WHERE forumid=?");
	query.addBindValue(sub->parser());

	if (query.exec()) {
		while (query.next()) {
			ForumGroup g(sub);
			//g.parser = query.value(0).toInt();
			g.setId(query.value(1).toString());
			g.setName(query.value(2).toString());
			g.setLastchange(query.value(3).toString());
			g.setSubscribed(query.value(4).toBool());
			g.setChangeset(query.value(5).toInt());
			if(!groups[sub].contains(g.id())) {
				ForumGroup *fg = new ForumGroup(sub);
				*fg = g;
				groups[sub][g.id()] = fg;
				emit groupFound(fg);
				listThreads(fg);
			}
		}
	} else {
		qDebug() << "Unable to list groups: " << query.lastError().text();
	}

	return groups[sub].values();
}

// @todo deprecate
/*
QList<ForumGroup*> ForumDatabase::listGroups(const int parser) {
	ForumSubscription *sub = getSubscription(parser);
	Q_ASSERT(sub);
	return listGroups(sub);
}
*/

ForumGroup* ForumDatabase::getGroup(const int forum, QString id) {
	return groups[subscriptions[forum]][id];
}

QList<ForumThread*> ForumDatabase::listThreads(ForumGroup *group) {
	QSqlQuery query;
	query.prepare(
			"SELECT DISTINCT * FROM threads WHERE forumid=? AND groupid=? ORDER BY ordernum");
	query.addBindValue(group->subscription()->parser());
	query.addBindValue(group->id());

	if (query.exec()) {
		while (query.next()) {
			ForumThread t(group);
			t.setId(query.value(2).toString());
			t.setOrdernum(query.value(3).toInt());
			t.setName(query.value(4).toString());
			t.setLastchange(query.value(5).toString());
			t.setChangeset(query.value(6).toInt());
			if(!threads[group].contains(t.id())) {
				ForumThread *ft = new ForumThread(group);
				*ft = t;
				threads[group][ft->id()] = ft;
				emit threadFound(ft);
				listMessages(ft);
			}
		}
	} else {
		qDebug() << "Unable to list threads: " << query.lastError().text();
	}

	return threads[group].values();
}

ForumThread* ForumDatabase::getThread(const int forum, QString groupid,
		QString threadid) {
	ForumSubscription *fs = subscriptions[forum];
	Q_ASSERT(fs);
	ForumGroup *fg = *groups[fs].find(groupid);
	Q_ASSERT(fg);
	return threads[fg][threadid];
/*
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
	*/
}

QList<ForumMessage*> ForumDatabase::listMessages(ForumThread *thread) {
	QSqlQuery query;
	query.prepare(
			"SELECT DISTINCT * FROM messages WHERE forumid=? AND groupid=? AND threadid=? ORDER BY ordernum");
	query.addBindValue(thread->group()->subscription()->parser());
	query.addBindValue(thread->group()->id());
	query.addBindValue(thread->id());
	if (query.exec()) {
		while (query.next()) {
			ForumMessage m(thread);
			//m.setForumid(query.value(0).toInt());
			//m.setGroupid(query.value(1).toString());
			//m.setThreadid(query.value(2).toString());
			m.setId(query.value(3).toString());
			m.setOrdernum(query.value(4).toInt());
			m.setUrl(query.value(5).toString());
			m.setSubject(query.value(6).toString());
			m.setAuthor(query.value(7).toString());
			m.setLastchange(query.value(8).toString());
			m.setBody(query.value(9).toString());
			m.setRead(query.value(10).toBool());
			if(!messages[thread][m.id()]) {
				ForumMessage *fm = new ForumMessage(thread);
				*fm = m;
				messages[thread][fm->id()] = fm;
				emit messageFound(fm);
			}
		}
	} else {
		qDebug() << "Unable to list messages: " << query.lastError().text();
	}
	return messages[thread].values();
}

ForumGroup* ForumDatabase::addGroup(const ForumGroup *grp) {
	Q_ASSERT(grp);
	Q_ASSERT(grp->isSane());
	Q_ASSERT(!groups[grp->subscription()].contains(grp->id()));
	QSqlQuery query;
	query.prepare(
			"INSERT INTO groups(forumid, groupid, name, lastchange, subscribed, changeset) VALUES (?, ?, ?, ?, ?, ?)");
	query.addBindValue(grp->subscription()->parser());
	query.addBindValue(grp->id());
	query.addBindValue(grp->name());
	query.addBindValue(grp->lastchange());
	query.addBindValue(grp->subscribed());
	query.addBindValue(grp->changeset());
	if (!query.exec()) {
		qDebug() << "Adding group failed: " << query.lastError().text();
		return 0;
	}
	Q_ASSERT(subscriptions[grp->subscription()->parser()] == grp->subscription());
	ForumGroup *nfg = new ForumGroup(grp->subscription());
	nfg->operator=(*grp);
	groups[grp->subscription()][nfg->id()] = nfg;
	emit groupAdded(nfg);
	emit groupFound(nfg);

	qDebug() << "Group " << nfg->toString() << " stored";
	return nfg;
}

ForumThread * ForumDatabase::addThread(const ForumThread *thread) {
	Q_ASSERT(thread);
	Q_ASSERT(thread->isSane());
	Q_ASSERT(groups[thread->group()->subscription()][thread->group()->id()] == thread->group());
	QSqlQuery query;
	query.prepare(
			"SELECT * FROM threads WHERE forumid=? AND groupid=? AND threadid=?");
	query.addBindValue(thread->group()->subscription()->parser());
	query.addBindValue(thread->group()->id());
	query.addBindValue(thread->id());

	query.exec();
	if (query.next()) {
		qDebug() << "Trying to add duplicate thread! " << thread->toString();
		return 0;
	}

	query.prepare(
			"INSERT INTO threads(forumid, groupid, threadid, name, ordernum, lastchange, changeset) VALUES (?, ?, ?, ?, ?, ?, ?)");
	query.addBindValue(thread->group()->subscription()->parser());
	query.addBindValue(thread->group()->id());
	query.addBindValue(thread->id());
	query.addBindValue(thread->name());
	query.addBindValue(thread->ordernum());
	query.addBindValue(thread->lastchange());
	query.addBindValue(thread->changeset());
	if (!query.exec()) {
		qDebug() << "Adding thread " << thread->toString() << " failed: "
				<< query.lastError().text();
		return 0;
	}
	ForumGroup *grp = thread->group();
	Q_ASSERT(!threads[grp].contains(thread->id()));
	ForumThread *th = new ForumThread(grp);
	th->operator=(*thread);
	threads[grp][th->id()] = th;
	emit threadAdded(th);
	emit threadFound(th);
	qDebug() << "Thread " << th->toString() << " stored";
	return th;
}

bool ForumDatabase::updateThread(ForumThread *thread) {
	Q_ASSERT(thread);
	Q_ASSERT(threads[thread->group()][thread->id()] == thread);
	Q_ASSERT(thread->isSane());
	QSqlQuery query;
	query.prepare(
			"UPDATE threads SET name=?, ordernum=?, lastchange=?, changeset=? WHERE(forumid=? AND groupid=? AND threadid=?)");
	query.addBindValue(thread->name());
	query.addBindValue(thread->ordernum());
	query.addBindValue(thread->lastchange());
	query.addBindValue(thread->changeset());
	query.addBindValue(thread->group()->subscription()->parser());
	query.addBindValue(thread->group()->id());
	query.addBindValue(thread->id());
	if (!query.exec()) {
		qDebug() << "Updating thread " << thread->toString() << " failed: "
				<< query.lastError().text();
		return false;
	}
	emit threadUpdated(thread);
	qDebug() << "Thread " << thread->toString() << " updated";
	return true;
}

ForumMessage* ForumDatabase::addMessage(ForumMessage *message) {
	Q_ASSERT(message);
	Q_ASSERT(message->isSane());
	Q_ASSERT(threads[message->thread()->group()][message->thread()->id()] == message->thread() );
	Q_ASSERT(!messages[message->thread()][message->id()]);
	QSqlQuery query;
	query.prepare(
			"SELECT * FROM messages WHERE forumid=? AND groupid=? AND threadid=? AND messageid=?");
	query.addBindValue(message->thread()->group()->subscription()->parser());
	query.addBindValue(message->thread()->group()->id());
	query.addBindValue(message->thread()->id());
	query.addBindValue(message->id());
	query.exec();
	if (query.next()) {
		qDebug() << "Looks like you're adding duplicate message "
				<< message->toString();
		return 0;
	}

	// QSqlQuery query;
	query.prepare("INSERT INTO messages(forumid, groupid, threadid, messageid,"
		" ordernum, url, subject, author, lastchange, body, read) VALUES "
		"(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
	bindMessageValues(query, message);

	if (!query.exec()) {
		qDebug() << "Adding message " << message->toString() << " failed: "
				<< query.lastError().text();
		return 0;
	}
	ForumMessage *nmsg = new ForumMessage(message->thread());
	nmsg->operator=(*message);
	messages[nmsg->thread()][nmsg->id()] = nmsg;
	emit messageAdded(nmsg);
	emit messageFound(nmsg);
	qDebug() << "Message " << nmsg->toString() << " stored";
	return nmsg;
}

ForumMessage* ForumDatabase::getMessage(const int forum, QString groupid,
		QString threadid, QString messageid) {
	ForumThread *thr = getThread(forum, groupid, threadid);
	Q_ASSERT(thr);
	return messages[thr][messageid];
/*
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
	*/
}

void ForumDatabase::bindMessageValues(QSqlQuery &query,
		const ForumMessage *message) {
	query.addBindValue(message->thread()->group()->subscription()->parser());
	query.addBindValue(message->thread()->group()->id());
	query.addBindValue(message->thread()->id());
	query.addBindValue(message->id());
	query.addBindValue(message->ordernum());
	query.addBindValue(message->url());
	query.addBindValue(message->subject());
	query.addBindValue(message->author());
	query.addBindValue(message->lastchange());
	query.addBindValue(message->body());
	query.addBindValue(message->read());
}

bool ForumDatabase::updateMessage(ForumMessage *message) {
	Q_ASSERT(message);
	Q_ASSERT(messages[message->thread()][message->id()]==message);
	QSqlQuery query;
	query.prepare(
			"UPDATE messages SET forumid=?, groupid=?, threadid=?, messageid=?,"
				" ordernum=?, url=?, subject=?, author=?, lastchange=?, body=?, read=? WHERE(forumid=? AND groupid=? AND threadid=? AND messageid=?)");

	bindMessageValues(query, message);
	query.addBindValue(message->thread()->group()->subscription()->parser());
	query.addBindValue(message->thread()->group()->id());
	query.addBindValue(message->thread()->id());
	query.addBindValue(message->id());
	if (!query.exec()) {
		qDebug() << "Updating message failed: " << query.lastError().text();
		Q_ASSERT(false);
		return false;
	}
	emit messageUpdated(message);
	qDebug() << "Message " << message->toString() << " updated";
	return true;
}

bool ForumDatabase::deleteMessage(ForumMessage *message) {
	Q_ASSERT(message);
	Q_ASSERT(message->isSane());
	QSqlQuery query;
	query.prepare(
			"DELETE FROM messages WHERE (forumid=? AND groupid=? AND threadid=? AND messageid=?)");
	query.addBindValue(message->thread()->group()->subscription()->parser());
	query.addBindValue(message->thread()->group()->id());
	query.addBindValue(message->thread()->id());
	query.addBindValue(message->id());
	if (!query.exec()) {
		qDebug() << "Deleting message failed: " << query.lastError().text();
		return false;
	}
        emit messageDeleted(message);
        messages[message->thread()].remove(message->id());
	qDebug() << "Message " << message->toString() << " deleted";
	message->deleteLater();
	return true;
}

bool ForumDatabase::deleteGroup(ForumGroup *grp) {
	Q_ASSERT(grp);
	Q_ASSERT(grp->isSane());
	Q_ASSERT(groups[grp->subscription()].contains(grp->id()));

	foreach(ForumThread *ft, threads[grp]){
		deleteThread(ft);
	}

	QSqlQuery query;
	query.prepare("DELETE FROM groups WHERE (forumid=? AND groupid=?)");
	query.addBindValue(grp->subscription()->parser());
	query.addBindValue(grp->id());
	if (!query.exec()) {
		qDebug() << "Deleting group failed: " << query.lastError().text();
		return false;
	}
	groups[grp->subscription()].remove(grp->id());
	emit groupDeleted(grp);
	qDebug() << "Group " << grp->toString() << " deleted";
	grp->deleteLater();
	return true;
}

bool ForumDatabase::deleteThread(ForumThread *thread) {
	Q_ASSERT(thread);
	Q_ASSERT(thread->isSane());
	if (!thread->isSane()) {
		qDebug() << "Error: tried to delete invalid tread "
				<< thread->toString();
	}

	foreach(ForumMessage *fm, messages[thread]){
		deleteMessage(fm);
	}

	QSqlQuery query;
	query.prepare(
			"DELETE FROM threads WHERE (forumid=? AND groupid=? AND threadid=?)");
	query.addBindValue(thread->group()->subscription()->parser());
	query.addBindValue(thread->group()->id());
	query.addBindValue(thread->id());
	if (!query.exec()) {
		qDebug() << "Deleting thread failed: " << query.lastError().text();
		return false;
	}
	threads[thread->group()].remove(thread->id());
	emit threadDeleted(thread);
	qDebug() << "Thread " << thread->toString() << " deleted";
	thread->deleteLater();
	return true;
}

bool ForumDatabase::updateGroup(ForumGroup *grp) {
	Q_ASSERT(grp->isSane());
	QSqlQuery query;
	query.prepare(
			"UPDATE groups SET name=?, lastchange=?, subscribed=?, changeset=? WHERE(forumid=? AND groupid=?)");
	query.addBindValue(grp->name());
	query.addBindValue(grp->lastchange());
	query.addBindValue(grp->subscribed());
	query.addBindValue(grp->changeset());
	query.addBindValue(grp->subscription()->parser());
	query.addBindValue(grp->id());
	if (!query.exec()) {
		qDebug() << "Updating group failed: " << query.lastError().text();
		return false;
	}
	qDebug() << "Group " << grp->toString() << " updated, subscribed:"
			<< grp->subscribed();
	emit groupUpdated(grp);
	return true;
}

void ForumDatabase::markMessageRead(ForumMessage *message) {

    if(message) {
        markMessageRead(message, true);
    }
}

void ForumDatabase::markMessageRead(ForumMessage *message, bool read) {
	Q_ASSERT(message);
	qDebug() << Q_FUNC_INFO;
        Q_ASSERT(message->isSane());

	QSqlQuery query;
	query.prepare(
			"UPDATE messages SET read=? WHERE(forumid=? AND groupid=? AND threadid=? AND messageid=?)");
	query.addBindValue(read);
	query.addBindValue(message->thread()->group()->subscription()->parser());
	query.addBindValue(message->thread()->group()->id());
	query.addBindValue(message->thread()->id());
	query.addBindValue(message->id());
	if (!query.exec()) {
		qDebug() << "Setting message read failed: " << query.lastError().text();
	}
	message->setRead(read);
	emit messageUpdated(message);
}

int ForumDatabase::unreadIn(const ForumSubscription *fs) {
	Q_ASSERT(fs);

	QSqlQuery query;
	query.prepare(
			"SELECT count() FROM messages WHERE forumid=? AND read=\"false\"");
	query.addBindValue(fs->parser());

	query.exec();
	if (query.next()) {
		return query.value(0).toInt();
	} else {
		qDebug() << "Can't count unreads in " << fs->toString();
	}

	return -1;
}

int ForumDatabase::unreadIn(const ForumGroup *fg) {
	Q_ASSERT(fg);
	QSqlQuery query;
	query.prepare(
			"SELECT count() FROM messages WHERE forumid=? AND groupid=? AND read=\"false\"");
	query.addBindValue(fg->subscription()->parser());
	query.addBindValue(fg->id());

	query.exec();
	if (query.next()) {
		return query.value(0).toInt();
	} else {
		qDebug() << "Can't count unreads in " << fg->toString();
	}

	return -1;
}

ForumSubscription *ForumDatabase::getSubscription(int id) {
	Q_ASSERT(id > 0);
	return subscriptions[id];
}

void ForumDatabase::markForumRead(ForumSubscription *fs, bool read) {
	Q_ASSERT(fs);
	foreach(ForumGroup *group, listGroups(fs))
		{
			markGroupRead(group, read);
		}
}

bool ForumDatabase::markGroupRead(ForumGroup *group, bool read) {
	Q_ASSERT(group);
	qDebug() << Q_FUNC_INFO << " " << group->toString() << ", " << read;
	QSqlQuery query;
	query.prepare("UPDATE messages SET read=? WHERE(forumid=? AND groupid=?)");
	query.addBindValue(read);
	query.addBindValue(group->subscription()->parser());
	query.addBindValue(group->id());
	if (!query.exec()) {
		qDebug() << "Setting group read failed: " << query.lastError().text();
		return false;
	}
	foreach(ForumThread *ft, listThreads(group)) {
		foreach(ForumMessage *msg, listMessages(ft)) {
			msg->setRead(read);
			emit messageUpdated(msg);
		}
                emit threadUpdated(ft);
	}
        emit groupUpdated(group);
        return true;
}

int ForumDatabase::schemaVersion() {
	return 1;
}
