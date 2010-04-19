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

//#define FDB_TEST 1

class ForumDatabase : public QObject {
    Q_OBJECT

public:
    ForumDatabase(QObject *parent);
    virtual ~ForumDatabase();
    bool openDatabase();
    void resetDatabase();
    ForumSubscription* addForum(ForumSubscription *fs);
    bool deleteForum(ForumSubscription *sub);
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

    int unreadIn(const ForumSubscription *fs);
    int unreadIn(const ForumGroup *fg);
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
    void threadDeleted(ForumThread *thr);
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
