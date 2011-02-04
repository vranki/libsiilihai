/* This file is part of libSiilihai.

    libSiilihai is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libSiilihai is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with libSiilihai.  If not, see <http://www.gnu.org/licenses/>. */

#include "forumdatabase.h"

ForumDatabase::ForumDatabase(QObject *parent) :
    QObject(parent), db(0) {
}

ForumDatabase::~ForumDatabase() {
    storeDatabase();
    disconnect(this);
}

void ForumDatabase::resetDatabase() {
    QSqlQuery query;
    query.exec("DROP TABLE forums");
    query.exec("DROP TABLE groups");
    query.exec("DROP TABLE threads");
    query.exec("DROP TABLE messages");
    query.exec("DROP INDEX idx_messages");
}

bool ForumDatabase::openDatabase(QSqlDatabase *database) {
    db = database;
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
                        "getmessagescount INTEGER, "
                        "lastpage INTEGER, "
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
            ForumSubscription *fs = new ForumSubscription(this, false);
            fs->setParser(query.value(0).toInt());
            fs->setAlias(query.value(1).toString());
            fs->setUsername(query.value(2).toString());
            fs->setPassword(query.value(3).toString());
            fs->setLatestThreads(query.value(4).toInt());
            fs->setLatestMessages(query.value(5).toInt());
            fs->setAuthenticated(fs->username().length() > 0);
            connect(fs, SIGNAL(changed(ForumSubscription*)), this, SLOT(subscriptionChanged(ForumSubscription*)));
            // connect(fs, SIGNAL(destroyed(QObject*)), this, SLOT(subscriptionDeleted(QObject*)));
            subscriptions[fs->parser()] = fs;
            emit subscriptionFound(fs);
            Q_ASSERT(getSubscription(fs->parser()));
        }
    } else {
        qDebug() << "Error listing subscriptions!";
        return false;
    }
    checkSanity();
    foreach(ForumSubscription *sub, subscriptions) {
        // Load groups
        query.prepare("SELECT groupid,name,lastchange,subscribed,changeset FROM groups WHERE forumid=?");
        query.addBindValue(sub->parser());

        if (query.exec()) {
            while (query.next()) {
                ForumGroup *g = new ForumGroup(sub, false);
                g->setId(query.value(0).toString());
                g->setName(query.value(1).toString());
                g->setLastchange(query.value(2).toString());
                g->setSubscribed(query.value(3).toBool());
                g->setChangeset(query.value(4).toInt());
                connect(g, SIGNAL(changed(ForumGroup*)), this, SLOT(groupChanged(ForumGroup*)));
                sub->groups().insert(g->id(), g);
                emit groupFound(g);
                Q_ASSERT(getGroup(sub, g->id()));
            }
        } else {
            qDebug() << "Unable to list groups: " << query.lastError().text();
            return false;
        }
    }
    checkSanity();

    // Load threads
    foreach(ForumSubscription *sub, subscriptions) {
        foreach(ForumGroup *grp, sub->groups()) {
            query.prepare("SELECT threadid,ordernum,name,lastchange,changeset,hasmoremessages,getmessagescount,lastpage FROM threads WHERE forumid=? AND groupid=? ORDER BY ordernum");
            query.addBindValue(grp->subscription()->parser());
            query.addBindValue(grp->id());

            if (query.exec()) {
                while (query.next()) {
                    ForumThread *t = new ForumThread(grp, false);
                    t->setId(query.value(0).toString());
                    t->setOrdernum(query.value(1).toInt());
                    t->setName(query.value(2).toString());
                    t->setLastchange(query.value(3).toString());
                    t->setChangeset(query.value(4).toInt());
                    t->setHasMoreMessages(query.value(5).toBool());
                    t->setGetMessagesCount(query.value(6).toInt());
                    t->setLastPage(query.value(7).toInt());
                    grp->threads().insert(t->id(), t);
                    connect(t, SIGNAL(changed(ForumThread*)), this, SLOT(threadChanged(ForumThread*)));
                    emit threadFound(t);
                    Q_ASSERT(getThread(t->group()->subscription()->parser(), grp->id(), t->id()));
                }
            } else {
                qDebug() << "Unable to list threads: " << query.lastError().text();
                return false;
            }
        }
    }
    checkSanity();

    // Load messages
    foreach(ForumSubscription *sub, subscriptions) {
        foreach(ForumGroup *grp, sub->groups()) {
            foreach(ForumThread *thread, grp->threads()) {
                // qDebug() << Q_FUNC_INFO << "Listing messages in " << thread->toString();
                query.prepare("SELECT messageid,ordernum,url,subject,author,lastchange,body,read FROM messages WHERE "\
                              "forumid=? AND groupid=? AND threadid=? ORDER BY ordernum");
                query.addBindValue(thread->group()->subscription()->parser());
                query.addBindValue(thread->group()->id());
                query.addBindValue(thread->id());
                if (query.exec()) {
                    while (query.next()) {
                        ForumMessage *m = new ForumMessage(thread, false);
                        m->setId(query.value(0).toString());
                        m->setOrdernum(query.value(1).toInt());
                        m->setUrl(query.value(2).toString());
                        m->setName(query.value(3).toString());
                        m->setAuthor(query.value(4).toString());
                        m->setLastchange(query.value(5).toString());
                        m->setBody(query.value(6).toString());
                        bool msgRead = query.value(7).toBool();
                        m->setRead(msgRead);
                        thread->messages().insert(m->id(), m);
                        connect(m, SIGNAL(changed(ForumMessage*)), this, SLOT(messageChanged(ForumMessage*)));
                        connect(m, SIGNAL(markedRead(ForumMessage*, bool)), this, SLOT(messageMarkedRead(ForumMessage*, bool)));
                        emit messageFound(m);
                        Q_ASSERT(getMessage(m->thread()->group()->subscription()->parser(), m->thread()->group()->id(), m->thread()->id(), m->id()));
                        Q_ASSERT(!m->thread()->messages().isEmpty());
                    }
                } else {
                    qDebug() << "Unable to list messages: " << query.lastError().text();
                    return false;
                }
                Q_ASSERT(thread->unreadCount() == recalcUnreads(thread));
            }
            Q_ASSERT(grp->unreadCount() == recalcUnreads(grp));
        }
        Q_ASSERT(sub->unreadCount() == recalcUnreads(sub));
    }
    checkSanity();

#ifdef FDB_TEST
    testPhase = 0;
    connect(&testTimer, SIGNAL(timeout()), this, SLOT(updateTest()));
    testTimer.start(2000);
#endif

    return true;
}


bool ForumDatabase::addSubscription(ForumSubscription *fs) {
    Q_ASSERT(fs);
    qDebug() << Q_FUNC_INFO << fs->toString();
    Q_ASSERT(fs->isSane());
    if(subscriptions.value(fs->parser())) return false; // already sub'd

    Q_ASSERT(!subscriptions.value(fs->parser()));
    Q_ASSERT(!fs->isTemp());
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
    query.addBindValue(QString().number(fs->latestThreads()));
    query.addBindValue(QString().number(fs->latestMessages()));

    if (!query.exec()) {
        qDebug() << "Adding forum failed: " << query.lastError().text();
    }
    connect(fs, SIGNAL(changed(ForumSubscription*)), this, SLOT(subscriptionChanged(ForumSubscription*)));
    subscriptions[fs->parser()] = fs;
    emit subscriptionAdded(fs);
    emit subscriptionFound(fs);

    checkSanity();
    return true;
}

void ForumDatabase::subscriptionChanged(ForumSubscription *sub) {
    Q_ASSERT(sub);
    qDebug() << Q_FUNC_INFO << sub->toString();
    Q_ASSERT(sub->isSane());
    Q_ASSERT(subscriptions.value(sub->parser()));

    QSqlQuery query;
    query.prepare("UPDATE forums SET name=?, username=?, password=?, latest_threads=?, latest_messages=? WHERE(parser=?)");
    query.addBindValue(sub->alias());
    if (sub->username().isNull()) {
        query.addBindValue(QString(""));
        query.addBindValue(QString(""));
    } else {
        query.addBindValue(sub->username());
        query.addBindValue(sub->password());
    }
    query.addBindValue(sub->latestThreads());
    query.addBindValue(sub->latestMessages());
    // Where
    query.addBindValue(sub->parser());

    if (!query.exec()) {
        qDebug() << "Updating subscription " << sub->toString() << " failed: "
                 << query.lastError().text();
        Q_ASSERT(false);
    }
    checkSanity();
}


QList<ForumSubscription*> ForumDatabase::listSubscriptions() {
    QSqlQuery query;
    return subscriptions.values();
}

void ForumDatabase::deleteSubscription(ForumSubscription *sub) {
    Q_ASSERT(sub);
    Q_ASSERT(subscriptions.value(sub->parser()));

    while(!sub->groups().isEmpty())
        deleteGroup(sub->groups().begin().value());

    QSqlQuery query;
    query.prepare("DELETE FROM forums WHERE (parser=?)");
    query.addBindValue(sub->parser());

    if (!query.exec()) {
        qDebug() << "Unable to delete forum" << sub->toString() << ": " << query.lastError().text();
        Q_ASSERT(false);
    }

    subscriptions.erase(subscriptions.find(sub->parser()));
    qDebug() << Q_FUNC_INFO << "Subscription " << sub->toString() << " deleted";
    delete(sub);
    checkSanity();
}

ForumGroup* ForumDatabase::getGroup(ForumSubscription *fs, QString id) {
    ForumGroup *fg = fs->groups().value(id);
    return fg;
}

ForumThread* ForumDatabase::getThread(const int forum, QString groupid,
                                      QString threadid) {
    ForumSubscription *fs = subscriptions.value(forum);
    Q_ASSERT(fs);
    ForumGroup *fg = getGroup(fs, groupid);
    Q_ASSERT(fg);
    return fg->threads().value(threadid);
}

void ForumDatabase::addGroup(ForumGroup *grp) {
    checkSanity();
    Q_ASSERT(grp);
    qDebug() << Q_FUNC_INFO << grp->toString();
    Q_ASSERT(grp->isSane());
    Q_ASSERT(!grp->isTemp());
    ForumSubscription *sub = getSubscription(grp->subscription()->parser());
    Q_ASSERT(sub);
    Q_ASSERT(sub == grp->subscription());

    QSqlQuery query;
    query.prepare("DELETE FROM groups WHERE forumid=? AND groupid=?");
    query.addBindValue(sub->parser());
    query.addBindValue(grp->id());
    Q_ASSERT(query.exec());

    query.prepare("DELETE FROM threads WHERE forumid=? AND groupid=?");
    query.addBindValue(sub->parser());
    query.addBindValue(grp->id());
    Q_ASSERT(query.exec());

    query.prepare("DELETE FROM messages WHERE forumid=? AND groupid=?");
    query.addBindValue(sub->parser());
    query.addBindValue(grp->id());
    Q_ASSERT(query.exec());

    grp->commitChanges();

    query.prepare("INSERT INTO groups(forumid, groupid, name, lastchange, subscribed, changeset) VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(sub->parser());
    query.addBindValue(grp->id());
    query.addBindValue(grp->name());
    query.addBindValue(grp->lastchange());
    query.addBindValue(grp->subscribed());
    query.addBindValue(grp->changeset());
    if (!query.exec()) {
        qDebug() << "Adding group failed: " << query.lastError().text() << grp->toString();
        Q_ASSERT(0);
    }
    Q_ASSERT(subscriptions.value(sub->parser()) == grp->subscription());

    connect(grp, SIGNAL(changed(ForumGroup*)), this, SLOT(groupChanged(ForumGroup*)));
    sub->groups().insert(grp->id(), grp);
    emit groupAdded(grp);
    emit groupFound(grp);

    qDebug() << Q_FUNC_INFO << "Group " << grp->toString() << " stored";
    checkSanity();
}

void ForumDatabase::addThread(ForumThread *thread) {
    checkSanity();
    Q_ASSERT(thread);
    Q_ASSERT(!thread->isTemp());
    Q_ASSERT(!thread->group()->isTemp());
    Q_ASSERT(!thread->group()->subscription()->isTemp());
    qDebug() << Q_FUNC_INFO << thread->toString();
    Q_ASSERT(thread->isSane());
    Q_ASSERT(thread->group());
    ForumThread *dbThread = getThread(thread->group()->subscription()->parser(), thread->group()->id(), thread->id());
    Q_ASSERT(!dbThread);
    threadsNotInDatabase.insert(thread);
    thread->group()->threads().insert(thread->id(), thread);
    thread->group()->setHasChanged(true);
    connect(thread, SIGNAL(changed(ForumThread*)), this, SLOT(threadChanged(ForumThread*)));
    emit threadAdded(thread);
    emit threadFound(thread);
    qDebug() << "Thread " << thread->toString() << " added";
    checkSanity();
}

void ForumDatabase::threadChanged(ForumThread *thread) {
    changedThreads.insert(thread);
}

void ForumDatabase::updateThread(ForumThread *thread) {
    checkSanity();
    Q_ASSERT(thread);
    qDebug() << Q_FUNC_INFO << thread->toString();
    Q_ASSERT(thread->isSane());
    QSqlQuery query;
    query.prepare("UPDATE threads SET name=?, ordernum=?, lastchange=?, changeset=?, hasmoremessages=?, getmessagescount=?, "
                  "lastpage=? WHERE(forumid=? AND groupid=? AND threadid=?)");
    query.addBindValue(thread->name());
    query.addBindValue(thread->ordernum());
    query.addBindValue(thread->lastchange());
    query.addBindValue(thread->changeset());
    query.addBindValue(thread->hasMoreMessages());
    query.addBindValue(thread->getMessagesCount());
    query.addBindValue(thread->getLastPage());
    // Where
    query.addBindValue(thread->group()->subscription()->parser());
    query.addBindValue(thread->group()->id());
    query.addBindValue(thread->id());

    if (!query.exec()) {
        qDebug() << "Updating thread " << thread->toString() << " failed: " << query.lastError().text();
    }
    changedThreads.remove(thread);
    // qDebug() << "Thread " << thread->toString() << " updated";
    checkSanity();
}

void ForumDatabase::addMessage(ForumMessage *message) {
    checkSanity();
    Q_ASSERT(message);
    Q_ASSERT(message->isSane());
    Q_ASSERT(!message->isTemp());
    Q_ASSERT(!message->thread()->isTemp());
    Q_ASSERT(!message->thread()->group()->isTemp());
    Q_ASSERT(!message->thread()->group()->subscription()->isTemp());
    qDebug() << Q_FUNC_INFO << message->toString();
#ifdef SANITY_CHECKS
    ForumThread *thread = getThread(message->thread()->group()->subscription()->parser(),
                                    message->thread()->group()->id(), message->thread()->id());
    Q_ASSERT(thread);
    Q_ASSERT(message->thread() == thread);
    ForumMessage *dbMessage = getMessage(message->thread()->group()->subscription()->parser(),
                                         message->thread()->group()->id(), message->thread()->id(), message->id());
    Q_ASSERT(!dbMessage);
#endif
    messagesNotInDatabase.insert(message);
    message->thread()->messages().insert(message->id(), message);
    message->thread()->group()->setHasChanged(true);
    connect(message, SIGNAL(changed(ForumMessage*)), this, SLOT(messageChanged(ForumMessage*)));
    connect(message, SIGNAL(markedRead(ForumMessage*, bool)), this, SLOT(messageMarkedRead(ForumMessage*, bool)));
    if(!message->isRead()) message->thread()->incrementUnreadCount(1);
    emit messageAdded(message);
    emit messageFound(message);
    checkSanity();
    QCoreApplication::processEvents();
}

ForumMessage* ForumDatabase::getMessage(const int forum, QString groupid,
                                        QString threadid, QString messageid) {
    ForumThread *thr = getThread(forum, groupid, threadid);
    Q_ASSERT(thr);
    if(!thr) return 0;
    return thr->messages().value(messageid);
}

void ForumDatabase::bindMessageValues(QSqlQuery &query,
                                      const ForumMessage *message) {
    query.addBindValue(message->thread()->group()->subscription()->parser());
    query.addBindValue(message->thread()->group()->id());
    query.addBindValue(message->thread()->id());
    query.addBindValue(message->id());
    query.addBindValue(message->ordernum());
    query.addBindValue(message->url());
    query.addBindValue(message->name());
    query.addBindValue(message->author());
    query.addBindValue(message->lastchange());
    query.addBindValue(message->body());
    query.addBindValue(message->isRead());
}

void ForumDatabase::messageChanged(ForumMessage *message) {
    Q_ASSERT(message);
    changedMessages.insert(message);
    //    updateMessage(message);
}

void ForumDatabase::updateMessage(ForumMessage *message) {
    Q_ASSERT(message);
    checkSanity();
    qDebug() << Q_FUNC_INFO << message->toString();
    QSqlQuery query;
    query.prepare("UPDATE messages SET forumid=?, groupid=?, threadid=?, messageid=?,"
                  " ordernum=?, url=?, subject=?, author=?, lastchange=?, body=?, read=? "
                  "WHERE(forumid=? AND groupid=? AND threadid=? AND messageid=?)");

    bindMessageValues(query, message);
    query.addBindValue(message->thread()->group()->subscription()->parser());
    query.addBindValue(message->thread()->group()->id());
    query.addBindValue(message->thread()->id());
    query.addBindValue(message->id());
    if (!query.exec()) {
        qDebug() << "Updating message failed: " << query.lastError().text();
        Q_ASSERT(false);
    }
    changedMessages.remove(message);
    checkSanity();
}

bool ForumDatabase::deleteMessage(ForumMessage *message) {
    checkSanity();
    Q_ASSERT(message);
    Q_ASSERT(message->isSane());
    messagesNotInDatabase.remove(message);
    QSqlQuery query;
    query.prepare(
                "DELETE FROM messages WHERE (forumid=? AND groupid=? AND threadid=? AND messageid=?)");
    query.addBindValue(message->thread()->group()->subscription()->parser());
    query.addBindValue(message->thread()->group()->id());
    query.addBindValue(message->thread()->id());
    query.addBindValue(message->id());
    if (!query.exec()) {
        qDebug() << "Deleting message failed: " << query.lastError().text();
        Q_ASSERT(false);
        return false;
    }
    Q_ASSERT(message->thread()->messages().remove(message->id()));
    if(!message->isRead()) message->thread()->incrementUnreadCount(-1);
    message->thread()->group()->setHasChanged(true);
    changedMessages.remove(message);
    qDebug() << Q_FUNC_INFO << "Message " << message->toString() << " deleted";
    delete(message);
    checkSanity();
    return true;
}

bool ForumDatabase::deleteGroup(ForumGroup *grp) {
    Q_ASSERT(grp);
    Q_ASSERT(grp->isSane());
    qDebug() << Q_FUNC_INFO << grp->toString();
    while(!grp->threads().isEmpty())
        deleteThread(grp->threads().begin().value());

    QSqlQuery query;
    query.prepare("DELETE FROM groups WHERE (forumid=? AND groupid=?)");
    query.addBindValue(grp->subscription()->parser());
    query.addBindValue(grp->id());
    if (!query.exec()) {
        qDebug() << "Deleting group failed: " << query.lastError().text();
        Q_ASSERT(false);
        return false;
    }
    Q_ASSERT(grp->subscription()->groups().remove(grp->id()));
    delete(grp);
    checkSanity();
    return true;
}

bool ForumDatabase::deleteThread(ForumThread *thread) {
    checkSanity();
    Q_ASSERT(thread);
    Q_ASSERT(thread->isSane());
    if (!thread->isSane()) {
        qDebug() << "Error: tried to delete invalid tread "
                 << thread->toString();
    }

    QSqlQuery query;
    query.prepare("DELETE FROM messages WHERE (forumid=? AND groupid=? AND threadid=?)");
    query.addBindValue(thread->group()->subscription()->parser());
    query.addBindValue(thread->group()->id());
    query.addBindValue(thread->id());
    if (!query.exec()) {
        qDebug() << "Deleting messages from thread " << thread->toString()
                 << " failed: " << query.lastError().text();
        return false;
    }
    changedThreads.remove(thread);
    threadsNotInDatabase.remove(thread);

    QList<ForumMessage*> messages = thread->messages().values();
    qSort(messages);
    while(!messages.isEmpty()) {
        ForumMessage *lastMessage = messages.takeLast();
        Q_ASSERT(lastMessage);
        deleteMessage(lastMessage);
    }

    query.prepare("DELETE FROM threads WHERE (forumid=? AND groupid=? AND threadid=?)");
    query.addBindValue(thread->group()->subscription()->parser());
    query.addBindValue(thread->group()->id());
    query.addBindValue(thread->id());
    if (!query.exec()) {
        qDebug() << "Deleting thread failed: " << query.lastError().text();
        Q_ASSERT(false);
        return false;
    }
    thread->group()->setHasChanged(true);
    Q_ASSERT(thread->group()->threads().remove(thread->id()));
    qDebug() << "Thread " << thread->toString() << " deleted";
    delete(thread);
    checkSanity();
    return true;
}

void ForumDatabase::groupChanged(ForumGroup *grp) {
    checkSanity();
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
    }
    //qDebug() << Q_FUNC_INFO << grp->toString() << " updated, subscribed:"
    //        << grp->subscribed();
    checkSanity();
}

void ForumDatabase::messageMarkedRead(ForumMessage *message, bool read) {

    checkSanity();
    Q_ASSERT(message);
    qDebug() << Q_FUNC_INFO << read;
    Q_ASSERT(message->isSane());
    checkSanity();
    message->thread()->group()->setHasChanged(true);
    return;
    /*
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
    checkSanity();*/
}
ForumSubscription *ForumDatabase::getSubscription(int id) {
    Q_ASSERT(id > 0);
    return subscriptions.value(id);
}

void ForumDatabase::markForumRead(ForumSubscription *fs, bool read) {
    Q_ASSERT(fs);
    checkSanity();
    foreach(ForumGroup *group, fs->groups())
    {
        markGroupRead(group, read);
    }
    checkSanity();
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
    foreach(ForumThread *ft, group->threads()) {
        foreach(ForumMessage *msg, ft->messages()) {
            msg->setRead(true);
        }
        Q_ASSERT(ft->unreadCount()==0);
    }
    Q_ASSERT(group->unreadCount()==0);
    checkSanity();
    return true;
}

int ForumDatabase::recalcUnreads(ForumThread * thr) {
    qDebug() << Q_FUNC_INFO << thr->toString() << this;
    int unreads = 0;
    foreach(ForumMessage * msg, thr->messages().values()) {
        qDebug() << Q_FUNC_INFO << msg->toString();
        if(!msg->isRead())
            unreads++;
    }
    thr->incrementUnreadCount(unreads - thr->unreadCount());
    return unreads;
}

int ForumDatabase::recalcUnreads(ForumGroup * grp) {
    int unreads = 0;
    foreach(ForumThread *thr, grp->threads().values()) {
        unreads += recalcUnreads(thr);
    }
    grp->incrementUnreadCount(unreads - grp->unreadCount());
    return unreads;
}

int ForumDatabase::recalcUnreads(ForumSubscription * sub) {
    int unreads = 0;
    foreach(ForumGroup * grp, sub->groups().values()) {
        unreads += recalcUnreads(grp);
    }
    sub->incrementUnreadCount(unreads - sub->unreadCount());
    return unreads;
}

int ForumDatabase::schemaVersion() {
    return 6;
}

void ForumDatabase::storeDatabase() {
    qDebug() << Q_FUNC_INFO;
    db->transaction();
    foreach (ForumThread *thread, threadsNotInDatabase) {
        qDebug() << Q_FUNC_INFO << "Storing thread " << thread->toString();
        QSqlQuery query;
        query.prepare("DELETE FROM threads WHERE forumid=? AND groupid=? AND threadid=?");
        query.addBindValue(thread->group()->subscription()->parser());
        query.addBindValue(thread->group()->id());
        query.addBindValue(thread->id());
        Q_ASSERT(query.exec());

        thread->commitChanges();

        // Delete all messages in this thread (if they exist for some reason)!
        // Otherwise they may cause havoc.
        query.prepare("DELETE FROM messages WHERE forumid=? AND groupid=? and THREADID=?");
        query.addBindValue(thread->group()->subscription()->parser());
        query.addBindValue(thread->group()->id());
        query.addBindValue(thread->id());
        if (!query.exec()) {
            qDebug() << Q_FUNC_INFO << "Clearing thread " << thread->toString() << " failed: "
                     << query.lastError().text();
            Q_ASSERT(false);
        } else if(query.numRowsAffected() > 0) {
            qDebug() << Q_FUNC_INFO << "Cleared " << query.numRowsAffected() << " rows in " << thread->toString();
        }

        query.prepare("INSERT INTO threads(forumid, groupid, threadid, name, ordernum, lastchange, "
                      "changeset, hasmoremessages, getmessagescount, lastpage) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        query.addBindValue(thread->group()->subscription()->parser());
        query.addBindValue(thread->group()->id());
        query.addBindValue(thread->id());
        query.addBindValue(thread->name());
        query.addBindValue(thread->ordernum());
        query.addBindValue(thread->lastchange());
        query.addBindValue(thread->changeset());
        query.addBindValue(thread->hasMoreMessages());
        query.addBindValue(thread->getMessagesCount());
        query.addBindValue(thread->getLastPage());
        if (!query.exec()) {
            qDebug() << Q_FUNC_INFO << "Adding thread " << thread->toString() << " failed: "
                     << query.lastError().text();
            Q_ASSERT(false);
        }
        changedThreads.remove(thread);
    }
    db->commit();
    db->transaction();
    foreach (ForumMessage *message, messagesNotInDatabase) {
        qDebug() << Q_FUNC_INFO << "Storing message " << message->toString();
        QSqlQuery query;

        // If database is in bad state, the message may already exist
        // although not in a known thread.
        query.prepare("DELETE FROM messages WHERE forumid=? AND groupid=? AND threadid=? AND messageid=?");
        query.addBindValue(message->thread()->group()->subscription()->parser());
        query.addBindValue(message->thread()->group()->id());
        query.addBindValue(message->thread()->id());
        query.addBindValue(message->id());
        Q_ASSERT(query.exec());

        message->commitChanges();

        query.prepare("INSERT INTO messages(forumid, groupid, threadid, messageid,"
                      " ordernum, url, subject, author, lastchange, body, read) VALUES "
                      "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        bindMessageValues(query, message);

        if (!query.exec()) {
            qDebug() << "Adding message " << message->toString() << " failed: "
                     << query.lastError().text();
            qDebug() << "Messages's thread: " << message->thread()->toString() << ", db thread: " << message->thread()->toString();
            Q_ASSERT(false);
        }
        changedMessages.remove(message);
    }
    db->commit();

    while(!changedThreads.isEmpty()) {
        ForumThread *thread = *changedThreads.begin();
        updateThread(thread);
    }
    while(!changedMessages.isEmpty()) {
        ForumMessage *message = *changedMessages.begin();
        updateMessage(message);
    }
    messagesNotInDatabase.clear();
    threadsNotInDatabase.clear();
    Q_ASSERT(changedMessages.isEmpty());
    Q_ASSERT(changedThreads.isEmpty());
}

void ForumDatabase::checkSanity() {
#ifdef SANITY_CHECKS
    foreach(ForumSubscription * sub, subscriptions) {
        Q_ASSERT(sub->isSane());
        Q_ASSERT(!sub->isTemp());
        QSet<QString> grp_ids;
        int unreadinforum = 0;
        foreach(ForumGroup * grp, sub->groups()) {
            Q_ASSERT(grp->isSane());
            Q_ASSERT(grp->subscription() == sub);
            Q_ASSERT(!grp_ids.contains(grp->id()));
            Q_ASSERT(!grp->isTemp());
            grp_ids.insert(grp->id());
            QSet<QString> thr_ids;
            int unreadingroup = 0;
            foreach(ForumThread * thr, grp->threads()) {
                Q_ASSERT(thr->isSane());
                Q_ASSERT(thr->group() == grp);
                Q_ASSERT(!thr_ids.contains(thr->id()));
                Q_ASSERT(!thr->isTemp());
                thr_ids.insert(thr->id());
                QSet<QString> msg_ids;
                int unreadinthread = 0;
                foreach(ForumMessage * msg, thr->messages()) {
                    Q_ASSERT(msg->isSane());
                    Q_ASSERT(msg->thread() == thr);
                    Q_ASSERT(!msg_ids.contains(msg->id()));
                    Q_ASSERT(!msg->isTemp());
                    msg_ids.insert(msg->id());

                    ForumMessage *dbmsg = getMessage(msg->thread()->group()->subscription()->parser(), msg->thread()->group()->id(), msg->thread()->id(), msg->id());
                    Q_ASSERT(dbmsg==msg);
                    if(!msg->isRead()) unreadinthread++;
                }
                Q_ASSERT(thr->unreadCount() == unreadinthread);
                unreadingroup += unreadinthread;
            }
            Q_ASSERT(grp->unreadCount()== unreadingroup);
            unreadinforum += unreadingroup;
        }
        Q_ASSERT(sub->unreadCount() == unreadinforum);
    }
#endif
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
