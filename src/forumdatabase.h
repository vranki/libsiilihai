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
    ForumSubscription* addSubscription(ForumSubscription *fs);
    bool updateSubscription(ForumSubscription *sub);
    bool deleteSubscription(ForumSubscription *sub);
    QList <ForumSubscription*> listSubscriptions();
    ForumSubscription* getSubscription(int id);

    QList <ForumGroup*> listGroups(ForumSubscription *fs);
    ForumGroup* getGroup(ForumSubscription *fs, QString id);
    ForumGroup* addGroup(const ForumGroup *grp);
    bool updateGroup(ForumGroup *grp);
    bool deleteGroup(ForumGroup *grp);

    QList <ForumThread*> listThreads(ForumGroup *group);
    ForumThread* getThread(const int forum, QString groupid, QString threadid);
    ForumThread* addThread(const ForumThread *thread);
    bool deleteThread(ForumThread *thread);
    bool updateThread(ForumThread *thread);

    QList <ForumMessage*> listMessages(ForumThread *thread);
    ForumMessage* getMessage(const int forum, QString groupid, QString threadid, QString messageid);
    ForumMessage * addMessage(ForumMessage *message);
    bool updateMessage(ForumMessage *message);
    bool deleteMessage(ForumMessage *message);

    int unreadIn(ForumSubscription *fs);
    int unreadIn(ForumGroup *fg);

    void markForumRead(ForumSubscription *fs, bool read);
    bool markGroupRead(ForumGroup *group, bool read);
    int schemaVersion();
public slots:
    void markMessageRead(ForumMessage *message);
    void markMessageRead(ForumMessage *message, bool read);

signals:
    void subscriptionAdded(ForumSubscription *sub);
    void subscriptionFound(ForumSubscription *sub);
    void subscriptionUpdated(ForumSubscription *sub);
    void subscriptionDeleted(ForumSubscription *sub);
    void groupAdded(ForumGroup *grp);
    void groupFound(ForumGroup *grp);
    void groupDeleted(ForumGroup *grp);
    void groupUpdated(ForumGroup *grp);
    void threadFound(ForumThread *thr);
    void threadAdded(ForumThread *thr);
    void threadDeleted(ForumThread *thr); // NOTE: RECURSIVE! Thread and messages in it deleted!
    void threadUpdated(ForumThread *thr);
    void messageFound(ForumMessage *msg);
    void messageAdded(ForumMessage *msg);
    void messageDeleted(ForumMessage *msg);
    void messageUpdated(ForumMessage *msg);
private:
    void bindMessageValues(QSqlQuery &query, const ForumMessage *message);
    QMap<int, ForumSubscription*> subscriptions;
    QMap<ForumSubscription*, QMap<QString, ForumGroup*> > groups;
    QMap<ForumGroup*, QMap<QString, ForumThread*> > threads;
    QMap<ForumThread*, QMap<QString, ForumMessage*> > messages;

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
