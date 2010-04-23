#include "forumdatabase.h"

ForumDatabase::ForumDatabase(QObject *parent) :
	QObject(parent) {
}

ForumDatabase::~ForumDatabase() {
}

bool ForumDatabase::deleteForum(ForumSubscription *sub) {
    Q_ASSERT(sub);
    Q_ASSERT(subscriptions[sub->parser()]);

    while(!groups.value(sub).isEmpty())
        deleteGroup(groups.value(sub).begin().value());

    Q_ASSERT(groups.value(sub).isEmpty());

    QSqlQuery query;
    query.prepare("DELETE FROM forums WHERE (parser=?)");
    query.addBindValue(sub->parser());

    if (!query.exec()) {
        qDebug() << "Unable to delete forum" << sub->toString() << ": " << query.lastError().text();
        Q_ASSERT(false);
        return false;
    }

    subscriptions.erase(subscriptions.find(sub->parser()));
    groups.erase(groups.find(sub));
    emit subscriptionDeleted(sub);
    qDebug() << "Subscription " << sub->toString() << " deleted";
    sub->deleteLater();
    return true;
}

ForumSubscription* ForumDatabase::addForum(ForumSubscription *fs) {
    Q_ASSERT(fs);
    qDebug() << Q_FUNC_INFO << fs->toString();
    Q_ASSERT(fs->isSane());
    Q_ASSERT(!subscriptions.value(fs->parser()));
    QSqlQuery query;
    query.prepare("INSERT INTO forums("
                  "parser, name, username, password, latest_threads, latest_messages"
                  ") VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(QString().number(fs->parser()));
    query.addBindValue(fs->alias());
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
    nfs->operator=(*fs);
    subscriptions[nfs->parser()] = nfs;
    emit subscriptionAdded(nfs);
    emit subscriptionFound(nfs);

    return nfs;
}

QList<ForumSubscription*> ForumDatabase::listSubscriptions() {
    QSqlQuery query;
    return subscriptions.values();
}

void ForumDatabase::resetDatabase() {
    QSqlQuery query;
    query.exec("DROP TABLE forums");
    query.exec("DROP TABLE groups");
    query.exec("DROP TABLE threads");
    query.exec("DROP TABLE messages");
    query.exec("DROP INDEX idx_messages");
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
                        "hasmoremessages BOOLEAN, "
                        "getallmessages BOOLEAN, "
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
        if (!query.exec("CREATE INDEX idx_messages ON messages(threadid)")) {
            qDebug() << "Couldn't create messages index!";
            return false;
        }
    }
    // Load subscriptions
    query.prepare("SELECT parser,name,username,password,latest_threads,latest_messages FROM forums");
    if (query.exec()) {
        while (query.next()) {
            ForumSubscription *fs = new ForumSubscription(this);
            fs->setParser(query.value(0).toInt());
            fs->setAlias(query.value(1).toString());
            fs->setUsername(query.value(2).toString());
            fs->setPassword(query.value(3).toString());
            fs->setLatestThreads(query.value(4).toInt());
            fs->setLatestMessages(query.value(5).toInt());
            subscriptions[fs->parser()] = fs;
            emit subscriptionFound(fs);
        }
    } else {
        qDebug() << "Error listing subscriptions!";
        return false;
    }
    foreach(ForumSubscription *sub, subscriptions) {
        // Load groups
        query.prepare("SELECT groupid,name,lastchange,subscribed,changeset FROM groups WHERE forumid=?");
        query.addBindValue(sub->parser());

        if (query.exec()) {
            while (query.next()) {
                ForumGroup *g = new ForumGroup(sub);
                g->setId(query.value(0).toString());
                g->setName(query.value(1).toString());
                g->setLastchange(query.value(2).toString());
                g->setSubscribed(query.value(3).toBool());
                g->setChangeset(query.value(4).toInt());
                groups[sub][g->id()] = g;
                emit groupFound(g);
            }
        } else {
            qDebug() << "Unable to list groups: " << query.lastError().text();
            return false;
        }
    }
    // Load threads
    foreach(ForumSubscription *sub, subscriptions) {
        foreach(ForumGroup *grp, groups[sub]) {
            query.prepare("SELECT threadid,ordernum,name,lastchange,changeset,hasmoremessages,getallmessages FROM threads WHERE forumid=? AND groupid=? ORDER BY ordernum");
            query.addBindValue(grp->subscription()->parser());
            query.addBindValue(grp->id());

            if (query.exec()) {
                while (query.next()) {
                    ForumThread *t = new ForumThread(grp);
                    t->setId(query.value(0).toString());
                    t->setOrdernum(query.value(1).toInt());
                    t->setName(query.value(2).toString());
                    t->setLastchange(query.value(3).toString());
                    t->setChangeset(query.value(4).toInt());
                    t->setHasMoreMessages(query.value(5).toBool());
                    t->setGetAllMessages(query.value(6).toBool());
                    Q_ASSERT(!threads[grp].contains(t->id()));
                    threads[grp][t->id()] = t;
                    emit threadFound(t);
                }
            } else {
                qDebug() << "Unable to list threads: " << query.lastError().text();
                return false;
            }
        }
    }
    // Load messages
    foreach(ForumSubscription *sub, subscriptions) {
        foreach(ForumGroup *grp, groups[sub]) {
            foreach(ForumThread *thread, threads[grp]) {
                qDebug() << "Listing messages in " << thread->toString();
                query.prepare("SELECT messageid,ordernum,url,subject,author,lastchange,body,read FROM messages WHERE "\
                              "forumid=? AND groupid=? AND threadid=? ORDER BY ordernum");
                query.addBindValue(thread->group()->subscription()->parser());
                query.addBindValue(thread->group()->id());
                query.addBindValue(thread->id());
                if (query.exec()) {
                    while (query.next()) {
                        ForumMessage *m = new ForumMessage(thread);
                        m->setId(query.value(0).toString());
                        m->setOrdernum(query.value(1).toInt());
                        m->setUrl(query.value(2).toString());
                        m->setSubject(query.value(3).toString());
                        m->setAuthor(query.value(4).toString());
                        m->setLastchange(query.value(5).toString());
                        m->setBody(query.value(6).toString());
                        m->setRead(query.value(7).toBool());
                        Q_ASSERT(!messages[thread][m->id()]);
                        messages[thread][m->id()] = m;
                        emit messageFound(m);
                    }
                } else {
                    qDebug() << "Unable to list messages: " << query.lastError().text();
                    return false;
                }
            }
        }
    }
#ifdef FDB_TEST
    testPhase = 0;
    connect(&testTimer, SIGNAL(timeout()), this, SLOT(updateTest()));
    testTimer.start(2000);
#endif

    return true;
}

QList <ForumGroup*> ForumDatabase::listGroups(ForumSubscription *sub) {
    if(groups.contains(sub)) {
        return groups.value(sub).values();
    } else {
        return QList<ForumGroup*>();
    }
}

ForumGroup* ForumDatabase::getGroup(ForumSubscription *fs, QString id) {
    ForumGroup *fg = 0;
    if(groups.contains(fs))
        if(groups.value(fs).contains(id))
            fg = groups.value(fs).value(id);
    return fg;
}

QList<ForumThread*> ForumDatabase::listThreads(ForumGroup *group) {
    if(threads.contains(group)) {
        return threads.value(group).values();
    } else {
        return QList<ForumThread*>();
    }
}

ForumThread* ForumDatabase::getThread(const int forum, QString groupid,
                                      QString threadid) {
    ForumSubscription *fs = subscriptions.value(forum);
    Q_ASSERT(fs);
    Q_ASSERT(groups.contains(fs));
    ForumGroup *fg = *groups.value(fs).find(groupid);
    Q_ASSERT(fg);

    if(threads.contains(fg)) {
        if(threads.value(fg).contains(threadid)) {
            return threads.value(fg).value(threadid);
        }
    }
    return 0;
}

QList<ForumMessage*> ForumDatabase::listMessages(ForumThread *thread) {
    if(messages.contains(thread)) {
        return messages.value(thread).values();
    } else {
        return QList<ForumMessage*>();
    }
}

ForumGroup* ForumDatabase::addGroup(const ForumGroup *grp) {
    Q_ASSERT(grp);
    qDebug() << Q_FUNC_INFO << grp->toString();
    Q_ASSERT(grp->isSane());
    ForumSubscription *sub = getSubscription(grp->subscription()->parser());
    Q_ASSERT(sub);
    Q_ASSERT(sub == grp->subscription());
    if(groups.contains(sub)) {
        Q_ASSERT(!groups.value(sub).contains(grp->id()));
    }

    QSqlQuery query;
    query.prepare(
            "INSERT INTO groups(forumid, groupid, name, lastchange, subscribed, changeset) VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(sub->parser());
    query.addBindValue(grp->id());
    query.addBindValue(grp->name());
    query.addBindValue(grp->lastchange());
    query.addBindValue(grp->subscribed());
    query.addBindValue(grp->changeset());
    if (!query.exec()) {
        qDebug() << "Adding group failed: " << query.lastError().text() << grp->toString();
        Q_ASSERT(0);
        return 0;
    }
    Q_ASSERT(subscriptions.value(sub->parser()) == grp->subscription());
    ForumGroup *nfg = new ForumGroup(sub);
    nfg->operator=(*grp);
    groups[sub][nfg->id()] = nfg;
    emit groupAdded(nfg);
    emit groupFound(nfg);

    qDebug() << "Group " << nfg->toString() << " stored";
    return nfg;
}

ForumThread * ForumDatabase::addThread(const ForumThread *thread) {
    Q_ASSERT(thread);
    qDebug() << Q_FUNC_INFO << thread->toString();
    Q_ASSERT(thread->isSane());
    Q_ASSERT(groups.contains(thread->group()->subscription()));
    Q_ASSERT(groups.value(thread->group()->subscription()).value(thread->group()->id()) == thread->group());
    Q_ASSERT(thread->group());
    QSqlQuery query;
    query.prepare(
            "SELECT * FROM threads WHERE forumid=? AND groupid=? AND threadid=?");
    query.addBindValue(thread->group()->subscription()->parser());
    query.addBindValue(thread->group()->id());
    query.addBindValue(thread->id());

    query.exec();
    if (query.next()) {
        qDebug() << "Trying to add duplicate thread! " << thread->toString();
        Q_ASSERT(false);
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
    Q_ASSERT(!threads.value(grp).contains(thread->id()));
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
    qDebug() << Q_FUNC_INFO << thread->toString();
    Q_ASSERT(threads.contains(thread->group()));
    Q_ASSERT(threads.value(thread->group()).value(thread->id()) == thread);
    Q_ASSERT(thread->isSane());
    QSqlQuery query;
    query.prepare(
            "UPDATE threads SET name=?, ordernum=?, lastchange=?, changeset=?, hasmoremessages=?, getallmessages=? WHERE(forumid=? AND groupid=? AND threadid=?)");
    query.addBindValue(thread->name());
    query.addBindValue(thread->ordernum());
    query.addBindValue(thread->lastchange());
    query.addBindValue(thread->changeset());
    query.addBindValue(thread->hasMoreMessages());
    query.addBindValue(thread->getAllMessages());
    // Where
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
    ForumThread *thread = getThread(message->thread()->group()->subscription()->parser(),
                                    message->thread()->group()->id(), message->thread()->id());
    Q_ASSERT(thread);
    Q_ASSERT(message->thread() == thread);
    if(messages.contains(thread)) {
        Q_ASSERT(!messages[thread].contains(message->id()));
    }
    QSqlQuery query;

    query.prepare(
            "SELECT * FROM messages WHERE forumid=? AND groupid=? AND threadid=? AND messageid=?");
    query.addBindValue(message->thread()->group()->subscription()->parser());
    query.addBindValue(message->thread()->group()->id());
    query.addBindValue(message->thread()->id());
    query.addBindValue(message->id());

    query.exec();
    if (query.next()) {
        qDebug() << "Trying to add duplicate message! " << thread->toString();
        Q_ASSERT(false);
        return 0;
    }
    query.prepare("INSERT INTO messages(forumid, groupid, threadid, messageid,"
                  " ordernum, url, subject, author, lastchange, body, read) VALUES "
                  "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    bindMessageValues(query, message);

    if (!query.exec()) {
        qDebug() << "Adding message " << message->toString() << " failed: "
                << query.lastError().text();
        qDebug() << "Messages's thread: " << message->thread()->toString() << ", db thread: " << thread->toString();
        return 0;
    }
    ForumMessage *nmsg = new ForumMessage(thread);
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
    if(messages.contains(thr)) {
        return messages.value(thr).value(messageid);
    } else {
        return 0;
    }
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
    Q_ASSERT(messages.contains(message->thread()));
    Q_ASSERT(messages.value(message->thread()).value(message->id())==message);
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

    while(!threads.value(grp).isEmpty())
        deleteThread(threads.value(grp).begin().value());

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

    while(!messages[thread].isEmpty())
        deleteMessage(messages[thread].begin().value());

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
    return subscriptions.value(id);
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
    return 3;
}

#ifdef FDB_TEST
void ForumDatabase::updateTest() {
    qDebug() << Q_FUNC_INFO << "Phase" << testPhase;
    if(testPhase==0) {
        testSub = new ForumSubscription(this);
        testSub->setParser(0);
        testSub->setAlias("Test Sub");
        testSub->setLatestThreads(20);
        testSub->setLatestMessages(20);
        subscriptions[0] = testSub;
        emit subscriptionFound(testSub);
    } else if(testPhase==1) {
        testGroup = new ForumGroup(testSub);
        testGroup->setName("A Group");
        testGroup->setId("a_group_id");
        testGroup->setSubscribed(true);
        groups[testSub][testGroup->id()] = testGroup;
        emit groupFound(testGroup);
    } else if(testPhase==2) {
        testThread = new ForumThread(testGroup);
        testThread->setName("A Thread");
        testThread->setId("a_thread_id");
        testThread->setOrdernum(0);
        threads[testGroup][testThread->id()] = testThread;
        emit threadFound(testThread);
    } else if(testPhase==3) {
        testMessage[0] = new ForumMessage(testThread);
        testMessage[0]->setSubject("A message");
        testMessage[0]->setId("a_message_id");
        testMessage[0]->setOrdernum(0);
        testMessage[0]->setAuthor("TestAuthor");
        testMessage[0]->setBody("Hello from test!");
        testMessage[0]->setRead(false);
        messages[testThread][testMessage[0]->id()] = testMessage[0];
        emit messageFound(testMessage[0]);
    } else if(testPhase==4) {
        testMessage[1] = new ForumMessage(testThread);
        testMessage[1]->setSubject("B message");
        testMessage[1]->setId("b_message_id");
        testMessage[1]->setOrdernum(1);
        testMessage[1]->setAuthor("TestAuthorB");
        testMessage[1]->setBody("Hello from test B!");
        testMessage[1]->setRead(false);
        messages[testThread][testMessage[1]->id()] = testMessage[1];
        emit messageFound(testMessage[1]);
    } else if(testPhase==5) {
        testMessage[2] = new ForumMessage(testThread);
        testMessage[2]->setSubject("C message");
        testMessage[2]->setId("c_message_id");
        testMessage[2]->setOrdernum(2);
        testMessage[2]->setAuthor("TestAuthorC");
        testMessage[2]->setBody("Hello from test C!");
        testMessage[2]->setRead(false);
        messages[testThread][testMessage[2]->id()] = testMessage[2];
        emit messageFound(testMessage[2]);
    } else if(testPhase==6) {
        testMessage[0]->setSubject("A Subject changed");
        testMessage[0]->setRead(true);
        emit messageUpdated(testMessage[0]);
    } else if(testPhase==7) {
        testMessage[2]->setSubject("C Moved to first in thread");
        testMessage[2]->setOrdernum(0);
        emit messageUpdated(testMessage[2]);
    } else if(testPhase==8) {
        testMessage[2]->setSubject("C Moved to 3rd");
        testMessage[2]->setOrdernum(3);
        emit messageUpdated(testMessage[2]);
    } else if(testPhase==9) {
        testMessage[1]->setSubject("B Moved to first");
        testMessage[1]->setOrdernum(0);
        emit messageUpdated(testMessage[1]);
    } else if(testPhase==10) {
        emit messageDeleted(testMessage[0]);
        messages[testThread].remove(testMessage[0]->id());
        testMessage[0]->deleteLater();
    } else if(testPhase==11) {
        emit messageDeleted(testMessage[2]);
        messages[testThread].remove(testMessage[2]->id());
        testMessage[2]->deleteLater();
    } else if(testPhase==12) {
        emit messageDeleted(testMessage[1]);
        messages[testThread].remove(testMessage[1]->id());
        testMessage[1]->deleteLater();
    } else if(testPhase==13) {
        emit threadDeleted(testThread);
        threads[testGroup].remove(testThread->id());
        testThread->deleteLater();
    } else if(testPhase==14) {
        emit groupDeleted(testGroup);
        groups[testSub].remove(testGroup->id());
        testGroup->deleteLater();
    } else if(testPhase==15) {
        emit subscriptionDeleted(testSub);
        subscriptions.remove(0);
        testSub->deleteLater();
    }
    testPhase++;
    if(testPhase==16) testPhase = 0;
}
#endif
