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

#ifndef FORUMDATABASE_H_
#define FORUMDATABASE_H_
#include <QObject>
#include <QtSql>
#include <QList>
#include <QMap>
#include <QTimer>
#include "forumsubscription.h"
#include "forumgroup.h"
#include "forumthread.h"
#include "forummessage.h"

// #define FDB_TEST 1

class ForumDatabase : public QObject {
    Q_OBJECT

public:
    ForumDatabase(QObject *parent);
    virtual ~ForumDatabase();
    bool openDatabase();
    void resetDatabase();
    void addSubscription(ForumSubscription *fs); // Ownership changes!!!
    QList <ForumSubscription*> listSubscriptions();
    ForumSubscription* getSubscription(int id);

    ForumGroup* getGroup(ForumSubscription *fs, QString id);
    void addGroup(ForumGroup *grp);
    bool deleteGroup(ForumGroup *grp);

    ForumThread* getThread(const int forum, QString groupid, QString threadid);
    void addThread(ForumThread *thread);
    bool deleteThread(ForumThread *thread);
    ForumMessage* getMessage(const int forum, QString groupid, QString threadid, QString messageid);
    void addMessage(ForumMessage *message);
    bool deleteMessage(ForumMessage *message);
    void markForumRead(ForumSubscription *fs, bool read);
    bool markGroupRead(ForumGroup *group, bool read);
    int schemaVersion();
    void deleteSubscription(ForumSubscription *sub);
public slots:
    //void subscriptionDeleted(QObject *sub);
    //void groupDeleted(QObject *g);
    //void threadDeleted(QObject *t);
    //void messageDeleted(QObject *m);

    void messageChanged(ForumMessage *message);
    void messageMarkedRead(ForumMessage *message, bool read=true);

    void threadChanged(ForumThread *thread);
    void groupChanged(ForumGroup *group);
    void subscriptionChanged(ForumSubscription *sub);

signals:
    void subscriptionAdded(ForumSubscription *sub);
    void subscriptionFound(ForumSubscription *sub);
    void subscriptionDeleted(ForumSubscription *sub);
    void groupAdded(ForumGroup *grp);
    void groupFound(ForumGroup *grp);
    //void groupDeleted(ForumGroup *grp);
    void threadFound(ForumThread *thr);
    void threadAdded(ForumThread *thr);
    void messageFound(ForumMessage *msg);
    void messageAdded(ForumMessage *msg);
private:
    void bindMessageValues(QSqlQuery &query, const ForumMessage *message);
    void checkSanity();
    QMap<int, ForumSubscription*> subscriptions;

#ifdef FDB_TEST
public slots:
    void updateTest();
private:
    QTimer testTimer;
    int testPhase;
    ForumSubscription *testSub;
    ForumGroup *testGroup;
    ForumThread *testThread;
    ForumMessage *testMessage[3];
#endif
};

#endif /* FORUMDATABASE_H_ */
