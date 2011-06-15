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

#include "forumdatabasesql.h"
#include "forummessage.h"
#include "forumthread.h"
#include "forumgroup.h"
#include "forumsubscription.h"

ForumDatabaseSql::ForumDatabaseSql(QObject *parent) : ForumDatabase(parent), db(0) {
    databaseOpened = false;
}

ForumDatabaseSql::~ForumDatabaseSql() {
}

void ForumDatabaseSql::resetDatabase() {
    QSqlQuery query;
    query.exec("DROP TABLE forums");
    query.exec("DROP TABLE groups");
    query.exec("DROP TABLE threads");
    query.exec("DROP TABLE messages");
    query.exec("DROP INDEX idx_messages");
}

bool ForumDatabaseSql::openDatabase(QSqlDatabase *database) {
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
            insert(fs->parser(), fs);
            emit subscriptionFound(fs);
            Q_ASSERT(value(fs->parser()));
        }
    } else {
        qDebug() << "Error listing subscriptions!";
        return false;
    }
    checkSanity();
    foreach(ForumSubscription *sub, values()) {
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
                sub->addGroup(g);
                //emit groupFound(g);
                Q_ASSERT(sub->contains(g->id()));
            }
        } else {
            qDebug() << "Unable to list groups: " << query.lastError().text();
            return false;
        }
        // Connect signals for subscription
        connect(sub, SIGNAL(changed(ForumSubscription*)), this, SLOT(subscriptionChanged(ForumSubscription*)));
        connect(sub, SIGNAL(groupAdded(ForumGroup*)), this, SLOT(addGroup(ForumGroup*)));
        connect(sub, SIGNAL(groupRemoved(ForumGroup*)), this, SLOT(addGroup(ForumGroup*)));
    }
    checkSanity();

    // Load threads
    foreach(ForumSubscription *sub, values()) {
        foreach(ForumGroup *grp, sub->values()) {
            query.prepare("SELECT threadid,ordernum,name,lastchange,changeset,hasmoremessages,getmessagescount,lastpage FROM threads WHERE forumid=? AND groupid=? ORDER BY ordernum");
            query.addBindValue(grp->subscription()->parser());
            query.addBindValue(grp->id());

            if (query.exec()) {
                while (query.next()) {
                    ForumThread *thread = new ForumThread(grp, false);
                    thread->setId(query.value(0).toString());
                    thread->setOrdernum(query.value(1).toInt());
                    thread->setName(query.value(2).toString());
                    thread->setLastchange(query.value(3).toString());
                    thread->setChangeset(query.value(4).toInt());
                    thread->setHasMoreMessages(query.value(5).toBool());
                    thread->setGetMessagesCount(query.value(6).toInt());
                    thread->setLastPage(query.value(7).toInt());
                    grp->addThread(thread, false);
                    //emit threadFound(thread);
                    Q_ASSERT(thread->unreadCount()==0);
                }
            } else {
                qDebug() << "Unable to list threads: " << query.lastError().text();
                return false;
            }
            connect(grp, SIGNAL(threadAdded(ForumThread*)), this, SLOT(addThread(ForumThread*)));
            connect(grp, SIGNAL(threadRemoved(ForumThread*)), this, SLOT(deleteThread(ForumThread*)));
            connect(grp, SIGNAL(changed(ForumGroup*)), this, SLOT(groupChanged(ForumGroup*)));
        }
    }
    checkSanity();

    // Load messages
    foreach(ForumSubscription *sub, values()) {
        foreach(ForumGroup *grp, sub->values()) {
            foreach(ForumThread *thread, grp->values()) {
                Q_ASSERT(thread->unreadCount()==0);
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
                        m->setRead(msgRead, false);
                        thread->addMessage(m, false);
                        connect(m, SIGNAL(changed(ForumMessage*)), this, SLOT(messageChanged(ForumMessage*)));
                        connect(m, SIGNAL(markedRead(ForumMessage*, bool)), this, SLOT(messageMarkedRead(ForumMessage*, bool)));
                        qDebug() << Q_FUNC_INFO << "Loaded message " << m->toString();
                        Q_ASSERT(!m->thread()->isEmpty());
                    }
                } else {
                    qDebug() << "Unable to list messages: " << query.lastError().text();
                    return false;
                }
                //Q_ASSERT(thread->unreadCount() == recalcUnreads(thread));
                connect(thread, SIGNAL(changed(ForumThread*)), this, SLOT(threadChanged(ForumThread*)));
                connect(thread, SIGNAL(messageRemoved(ForumMessage*)), this, SLOT(deleteMessage(ForumMessage*)));
                connect(thread, SIGNAL(messageAdded(ForumMessage*)), this, SLOT(addMessage(ForumMessage*)));
                if(thread->isEmpty()) // Force update if contains no messages
                    thread->markToBeUpdated();
            }
            if(grp->isEmpty()) // Force update if contains no messages
                grp->markToBeUpdated();
            //            Q_ASSERT(grp->unreadCount() == recalcUnreads(grp));
        }
        //      Q_ASSERT(sub->unreadCount() == recalcUnreads(sub));
    }
    checkSanity();

#ifdef FDB_TEST
    testPhase = 0;
    connect(&testTimer, SIGNAL(timeout()), this, SLOT(updateTest()));
    testTimer.start(2000);
#endif
    databaseOpened = true;
    return true;
}


bool ForumDatabaseSql::addSubscription(ForumSubscription *fs) {
    Q_ASSERT(fs);
    qDebug() << Q_FUNC_INFO << fs->toString();
    Q_ASSERT(fs->isSane());
    if(value(fs->parser())) return false; // already sub'd

    Q_ASSERT(!value(fs->parser()));
    Q_ASSERT(!fs->isTemp());
    QSqlQuery query;
    query.prepare("INSERT INTO forums("
                  "parser, name, username, password, latest_threads, latest_messages"
                  ") VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(QString().number(fs->parser()));
    query.addBindValue(fs->alias());
    if (fs->username().isNull() || fs->password().isNull()) {
        query.addBindValue(QString(""));
        query.addBindValue(QString(""));
    } else {
        query.addBindValue(fs->username());
        query.addBindValue(fs->password());
    }
    query.addBindValue(fs->latestThreads());
    query.addBindValue(fs->latestMessages());
    bool success = true;
    if (!query.exec()) {
        qDebug() << Q_FUNC_INFO << "Adding forum failed: " << query.lastError().text();
        success = false;
    }
    connect(fs, SIGNAL(changed(ForumSubscription*)), this, SLOT(subscriptionChanged(ForumSubscription*)));
    insert(fs->parser(), fs);
    emit subscriptionAdded(fs);
    emit subscriptionFound(fs);

    checkSanity();
    return success;
}

void ForumDatabaseSql::subscriptionChanged(ForumSubscription *sub) {
    Q_ASSERT(sub);
    qDebug() << Q_FUNC_INFO << sub->toString();
    Q_ASSERT(sub->isSane());
    Q_ASSERT(value(sub->parser()));

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

void ForumDatabaseSql::deleteSubscription(ForumSubscription *sub) {
    Q_ASSERT(sub);
    Q_ASSERT(value(sub->parser()));

    while(!sub->isEmpty())
        deleteGroup(sub->values().last());

    QSqlQuery query;
    query.prepare("DELETE FROM forums WHERE (parser=?)");
    query.addBindValue(sub->parser());

    if (!query.exec()) {
        qDebug() << "Unable to delete forum" << sub->toString() << ": " << query.lastError().text();
        Q_ASSERT(false);
    }

    remove(sub->parser());
    qDebug() << Q_FUNC_INFO << "Subscription " << sub->toString() << " deleted";
    sub->deleteLater();
    checkSanity();
}


void ForumDatabaseSql::addGroup(ForumGroup *grp) {
    checkSanity();
    Q_ASSERT(grp);
    qDebug() << Q_FUNC_INFO << grp->toString();
    Q_ASSERT(grp->isSane());
    Q_ASSERT(!grp->isTemp());
    ForumSubscription *sub = value(grp->subscription()->parser());
    Q_ASSERT(sub);
    Q_ASSERT(sub == grp->subscription());

    QSqlQuery query;
    query.prepare("DELETE FROM groups WHERE forumid=? AND groupid=?");
    query.addBindValue(sub->parser());
    query.addBindValue(grp->id());
    query.exec();

    query.prepare("DELETE FROM threads WHERE forumid=? AND groupid=?");
    query.addBindValue(sub->parser());
    query.addBindValue(grp->id());
    query.exec();

    query.prepare("DELETE FROM messages WHERE forumid=? AND groupid=?");
    query.addBindValue(sub->parser());
    query.addBindValue(grp->id());
    query.exec();

    grp->commitChanges();

    query.prepare("INSERT INTO groups(forumid, groupid, name, lastchange, subscribed, changeset) VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(sub->parser());
    query.addBindValue(grp->id());
    query.addBindValue(grp->name());
    query.addBindValue(grp->lastchange());
    query.addBindValue(grp->isSubscribed());
    query.addBindValue(grp->changeset());
    if (!query.exec()) {
        qDebug() << "Adding group failed: " << query.lastError().text() << grp->toString();
        Q_ASSERT(0);
    }
    Q_ASSERT(value(sub->parser()) == grp->subscription());

    connect(grp, SIGNAL(changed(ForumGroup*)), this, SLOT(groupChanged(ForumGroup*)));
    connect(grp, SIGNAL(threadAdded(ForumThread*)), this, SLOT(addThread(ForumThread*)));
    connect(grp, SIGNAL(threadRemoved(ForumThread*)), this, SLOT(deleteThread(ForumThread*)));
    //emit groupAdded(grp);
    //emit groupFound(grp);

    qDebug() << Q_FUNC_INFO << "Group " << grp->toString() << " stored";
    checkSanity();
}

bool ForumDatabaseSql::deleteGroup(ForumGroup *grp) {
    Q_ASSERT(grp);
    Q_ASSERT(grp->isSane());
    qDebug() << Q_FUNC_INFO << grp->toString();
    while(!grp->isEmpty()) {
        deleteThread(grp->begin().value());
        QCoreApplication::processEvents();
    }

    QSqlQuery query;
    query.prepare("DELETE FROM groups WHERE (forumid=? AND groupid=?)");
    query.addBindValue(grp->subscription()->parser());
    query.addBindValue(grp->id());
    if (!query.exec()) {
        qDebug() << "Deleting group failed: " << query.lastError().text();
        Q_ASSERT(false);
        return false;
    }
    grp->deleteLater();
    checkSanity();
    return true;
}
void ForumDatabaseSql::groupChanged(ForumGroup *grp) {
    checkSanity();
    Q_ASSERT(grp->isSane());
    db->transaction();
    QSqlQuery query;
    query.prepare("UPDATE groups SET name=?, lastchange=?, subscribed=?, changeset=? WHERE(forumid=? AND groupid=?)");
    query.addBindValue(grp->name());
    query.addBindValue(grp->lastchange());
    query.addBindValue(grp->isSubscribed());
    query.addBindValue(grp->changeset());
    query.addBindValue(grp->subscription()->parser());
    query.addBindValue(grp->id());
    if (!query.exec()) {
        qDebug() << Q_FUNC_INFO << "Updating group failed: " << query.lastError().text();
    }
    db->commit();
    checkSanity();
}

void ForumDatabaseSql::addThread(ForumThread *thread) {
    checkSanity();
    Q_ASSERT(thread);
    Q_ASSERT(!thread->isTemp());
    Q_ASSERT(!thread->group()->isTemp());
    Q_ASSERT(!thread->group()->subscription()->isTemp());
    qDebug() << Q_FUNC_INFO << thread->toString();
    Q_ASSERT(thread->isSane());
    Q_ASSERT(thread->group());
    threadsNotInDatabase.insert(thread);
    connect(thread, SIGNAL(changed(ForumThread*)), this, SLOT(threadChanged(ForumThread*)));
    connect(thread, SIGNAL(messageRemoved(ForumMessage*)), this, SLOT(deleteMessage(ForumMessage*)));
    connect(thread, SIGNAL(messageAdded(ForumMessage*)), this, SLOT(addMessage(ForumMessage*)));

    qDebug() << Q_FUNC_INFO << thread->toString();
    checkSanity();
}


bool ForumDatabaseSql::deleteThread(ForumThread *thread) {
    qDebug() << Q_FUNC_INFO;
    checkSanity();
    Q_ASSERT(thread);
    Q_ASSERT(thread->isSane());
    if (!thread->isSane()) {
        qDebug() << Q_FUNC_INFO << "Error: tried to delete invalid thread " << thread->toString();
    }
    db->transaction();
    QSqlQuery query;
    query.prepare("DELETE FROM messages WHERE (forumid=? AND groupid=? AND threadid=?)");
    query.addBindValue(thread->group()->subscription()->parser());
    query.addBindValue(thread->group()->id());
    query.addBindValue(thread->id());
    if (!query.exec()) {
        qDebug() << Q_FUNC_INFO << "Deleting messages from thread " << thread->toString()
                 << " failed: " << query.lastError().text();
        db->rollback();
        return false;
    }
    db->commit();
    changedThreads.remove(thread);
    threadsNotInDatabase.remove(thread);

    QList<ForumMessage*> messages = thread->values();
    qSort(messages);
    while(!messages.isEmpty()) {
        ForumMessage *lastMessage = messages.takeLast();
        Q_ASSERT(lastMessage);
        thread->removeMessage(lastMessage);
    }
    db->transaction();
    query.prepare("DELETE FROM threads WHERE (forumid=? AND groupid=? AND threadid=?)");
    query.addBindValue(thread->group()->subscription()->parser());
    query.addBindValue(thread->group()->id());
    query.addBindValue(thread->id());
    if (!query.exec()) {
        qDebug() << "Deleting thread failed: " << query.lastError().text();
        Q_ASSERT(false);
        db->rollback();
        return false;
    }
    db->commit();
    qDebug() << Q_FUNC_INFO << "Thread " << thread->toString() << " deleted";
    thread->deleteLater();
    checkSanity();
    return true;
}

void ForumDatabaseSql::threadChanged(ForumThread *thread) {
    changedThreads.insert(thread);
}

void ForumDatabaseSql::updateThread(ForumThread *thread) {
    checkSanity();
    Q_ASSERT(thread);
    Q_ASSERT(thread->isSane());
    db->transaction();
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
        qDebug() << Q_FUNC_INFO << "Updating thread " << thread->toString() << " failed: " << query.lastError().text();
    }
    db->commit();
    changedThreads.remove(thread);
    // qDebug() << "Thread " << thread->toString() << " updated";
    checkSanity();
}

// Note: Message is already in thread when this is called.
void ForumDatabaseSql::addMessage(ForumMessage *message) {
    if(!databaseOpened) return;
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
    Q_ASSERT(dbMessage);
#endif
    messagesNotInDatabase.insert(message);
    connect(message, SIGNAL(changed(ForumMessage*)), this, SLOT(messageChanged(ForumMessage*)));
    connect(message, SIGNAL(markedRead(ForumMessage*, bool)), this, SLOT(messageMarkedRead(ForumMessage*, bool)));
    //emit messageAdded(message);
    //emit messageFound(message);
    checkSanity();
}

void ForumDatabaseSql::deleteMessage(ForumMessage *message) {
    if(!databaseOpened) return;

    checkSanity();
    Q_ASSERT(message);
    Q_ASSERT(message->isSane());
    messagesNotInDatabase.remove(message);
    db->transaction();
    QSqlQuery query;
    query.prepare("DELETE FROM messages WHERE (forumid=? AND groupid=? AND threadid=? AND messageid=?)");
    query.addBindValue(message->thread()->group()->subscription()->parser());
    query.addBindValue(message->thread()->group()->id());
    query.addBindValue(message->thread()->id());
    query.addBindValue(message->id());
    if (!query.exec()) {
        qDebug() << "Deleting message failed: " << query.lastError().text();
        Q_ASSERT(false);
    }
    db->commit();
    message->thread()->group()->setHasChanged(true);
    changedMessages.remove(message);
    qDebug() << Q_FUNC_INFO << "Message " << message->toString() << " deleted";
    message->deleteLater();
    checkSanity();
}


void ForumDatabaseSql::bindMessageValues(QSqlQuery &query,
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

void ForumDatabaseSql::messageChanged(ForumMessage *message) {
    Q_ASSERT(message);
    changedMessages.insert(message);
}

void ForumDatabaseSql::updateMessage(ForumMessage *message) {
    Q_ASSERT(message);
    checkSanity();
    db->transaction();
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
    db->commit();
    changedMessages.remove(message);
    checkSanity();
}


void ForumDatabaseSql::messageMarkedRead(ForumMessage *message, bool read) {
    Q_UNUSED(read);
    checkSanity();
    Q_ASSERT(message);
    qDebug() << Q_FUNC_INFO << message->toString();
    Q_ASSERT(message->isSane());
    checkSanity();
    message->thread()->group()->setHasChanged(true);
    changedMessages.insert(message);
}

void ForumDatabaseSql::markForumRead(ForumSubscription *fs, bool read) {
    Q_ASSERT(fs);
    checkSanity();
    foreach(ForumGroup *group, fs->values()) {
        markGroupRead(group, read);
    }
    checkSanity();
}

bool ForumDatabaseSql::markGroupRead(ForumGroup *group, bool read) {
    Q_ASSERT(group);
    //    qDebug() << Q_FUNC_INFO << " " << group->toString() << ", " << read;
    foreach(ForumThread *ft, group->values()) {
        foreach(ForumMessage *msg, ft->values()) {
            msg->setRead(read);
        }
        QCoreApplication::processEvents();
    }
    checkSanity();
    return true;
}

int ForumDatabaseSql::schemaVersion() {
    return 6;
}

void ForumDatabaseSql::storeSomethingSmall() {
    if(!threadsNotInDatabase.isEmpty()) {
        db->transaction();
        ForumThread *thread = *threadsNotInDatabase.begin();
        qDebug() << Q_FUNC_INFO << "Storing thread " << thread->toString();
        QSqlQuery query;
        query.prepare("DELETE FROM threads WHERE forumid=? AND groupid=? AND threadid=?");
        query.addBindValue(thread->group()->subscription()->parser());
        query.addBindValue(thread->group()->id());
        query.addBindValue(thread->id());
        query.exec();

        thread->commitChanges();

        // Delete all messages in this thread (if they exist for some reason)!
        // Otherwise they may cause havoc.
        query.prepare("DELETE FROM messages WHERE forumid=? AND groupid=? and threadid=?");
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
        threadsNotInDatabase.remove(thread);
        db->commit();
        return;
    } else if(!messagesNotInDatabase.isEmpty()) {
        db->transaction();
        ForumMessage *message = *messagesNotInDatabase.begin();
        qDebug() << Q_FUNC_INFO << "Storing message " << message->toString();
        QSqlQuery query;

        // If database is in bad state, the message may already exist
        // although not in a known thread.
        query.prepare("DELETE FROM messages WHERE forumid=? AND groupid=? AND threadid=? AND messageid=?");
        query.addBindValue(message->thread()->group()->subscription()->parser());
        query.addBindValue(message->thread()->group()->id());
        query.addBindValue(message->thread()->id());
        query.addBindValue(message->id());
        query.exec();

        message->commitChanges();

        query.prepare("INSERT INTO messages(forumid, groupid, threadid, messageid,"
                      " ordernum, url, subject, author, lastchange, body, read) VALUES "
                      "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        bindMessageValues(query, message);

        if (!query.exec()) {
            qDebug() << Q_FUNC_INFO << "Adding message " << message->toString() << " failed: "
                     << query.lastError().text();
            qDebug() << "Messages's thread: " << message->thread()->toString() << ", db thread: " << message->thread()->toString();
            Q_ASSERT(false);
        }
        changedMessages.remove(message);
        messagesNotInDatabase.remove(message);
        db->commit();
        return;
    } else if(!changedThreads.isEmpty()) {
        ForumThread *thread = *changedThreads.begin();
        updateThread(thread);
        return;
    } else if(!changedMessages.isEmpty()) {
        ForumMessage *message = *changedMessages.begin();
        updateMessage(message);
        return;
    }
}

void ForumDatabaseSql::storeDatabase() {
    qDebug() << Q_FUNC_INFO;
    while(!isStored()) {
        storeSomethingSmall();
        QCoreApplication::processEvents();
    }
    Q_ASSERT(changedMessages.isEmpty());
    Q_ASSERT(changedThreads.isEmpty());
    Q_ASSERT(isStored());
    emit databaseStored();
}

bool ForumDatabaseSql::isStored() {
    return threadsNotInDatabase.isEmpty() && messagesNotInDatabase.isEmpty()
            && changedMessages.isEmpty() && changedThreads.isEmpty();
}


void ForumDatabaseSql::checkSanity() {
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
                foreach(ForumMessage * msg, thr->values()) {
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
