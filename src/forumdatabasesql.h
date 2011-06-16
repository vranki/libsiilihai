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
#include <QSqlDatabase>
#include "forumdatabase.h"

class ForumSubscription;
class ForumGroup;
class ForumThread;
class ForumMessage;

// #define FDB_TEST 1
// #define SANITY_CHECKS 1
/**
  * Handles storing the subscriptions, groups, threads and messages to
  * local sqlite database.
  *
  * @todo too large & complex
  */
class ForumDatabaseSql : public ForumDatabase {
    Q_OBJECT

public:
    ForumDatabaseSql(QObject *parent);
    virtual ~ForumDatabaseSql();
    virtual bool openDatabase(QSqlDatabase *database);
    virtual void resetDatabase();
    virtual int schemaVersion();
    virtual bool isStored();

    // Subscription related
    virtual bool addSubscription(ForumSubscription *fs); // Ownership changes!!!
    virtual void deleteSubscription(ForumSubscription *sub);

    virtual void markForumRead(ForumSubscription *fs, bool read);
    virtual bool markGroupRead(ForumGroup *group, bool read);

public slots:
    virtual void messageChanged(ForumMessage *message);
    virtual void messageMarkedRead(ForumMessage *message, bool read=true);
    virtual void threadChanged(ForumThread *thread);
    virtual void groupChanged(ForumGroup *group);
    virtual void subscriptionChanged(ForumSubscription *sub);
    virtual void storeDatabase();
    virtual void storeSomethingSmall();
    virtual void checkSanity();
private slots:
    void addThread(ForumThread *thread);
    bool deleteThread(ForumThread *thread);

    void deleteMessage(ForumMessage *message);
    void addMessage(ForumMessage *message);
    // Group related
    void addGroup(ForumGroup *grp);
    bool deleteGroup(ForumGroup *grp);
private:
    void bindMessageValues(QSqlQuery &query, const ForumMessage *message);
    void updateMessage(ForumMessage *message);
    void updateThread(ForumThread *thread);

    QSet<ForumMessage*> messagesNotInDatabase;
    QSet<ForumMessage*> changedMessages;
    QSet<ForumThread*> threadsNotInDatabase;
    QSet<ForumThread*> changedThreads;
    QSqlDatabase *db;
    bool databaseOpened;

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
