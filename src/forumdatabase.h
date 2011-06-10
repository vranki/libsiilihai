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
class ForumDatabase : public QObject, public QMap<int, ForumSubscription*> {
    Q_OBJECT

public:
    ForumDatabase(QObject *parent);
    virtual ~ForumDatabase();
    bool openDatabase(QSqlDatabase *database);
    void resetDatabase();
    int schemaVersion();
    bool isStored();

    // Subscription related
    bool addSubscription(ForumSubscription *fs); // Ownership changes!!!
    void deleteSubscription(ForumSubscription *sub);

    // Thread related
    ForumThread* getThread(const int forum, QString groupid, QString threadid);

    // Message related
    ForumMessage* getMessage(const int forum, QString groupid, QString threadid, QString messageid);

    void markForumRead(ForumSubscription *fs, bool read);
    bool markGroupRead(ForumGroup *group, bool read);

public slots:
    void messageChanged(ForumMessage *message);
    void messageMarkedRead(ForumMessage *message, bool read=true);
    void threadChanged(ForumThread *thread);
    void groupChanged(ForumGroup *group);
    void subscriptionChanged(ForumSubscription *sub);
    void storeDatabase();
    void storeSomethingSmall();
    void checkSanity();
signals:
    void subscriptionAdded(ForumSubscription *sub);
    void subscriptionFound(ForumSubscription *sub);

    void databaseStored();
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
