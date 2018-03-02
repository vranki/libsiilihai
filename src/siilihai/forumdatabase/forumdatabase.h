#ifndef FORUMDATABASE_H
#define FORUMDATABASE_H

#include <QObject>
#include <QList>
#include "../forumdata/grouplistmodel.h"

class ForumSubscription;
class ForumGroup;
class ForumThread;
class ForumMessage;

class ForumDatabase : public QObject, public QList<ForumSubscription*> {
    Q_OBJECT
    Q_PROPERTY(QList<QObject*> subscriptions READ subscriptions NOTIFY subscriptionsChanged)
    // Q_PROPERTY(QObject* groupListModel READ groupListModel NOTIFY groupListModelChanged)

public:
    explicit ForumDatabase(QObject *parent = 0);
    ~ForumDatabase();
    virtual void resetDatabase();
    virtual int schemaVersion()=0;
    virtual bool isStored()=0;
    virtual bool contains(int id);
    virtual bool contains(ForumSubscription *other);
    virtual ForumSubscription* findById(int id);

    // Subscription related
    virtual bool addSubscription(ForumSubscription *fs); // Ownership changes!!!
    virtual void deleteSubscription(ForumSubscription *sub);

    // Thread related
    virtual ForumThread* getThread(const int forum, QString groupid, QString threadid);

    // Message related
    virtual ForumMessage* getMessage(const int forum, QString groupid, QString threadid, QString messageid);

    virtual QList<QObject *> subscriptions();

    QObject* groupListModel();

public slots:
    virtual bool storeDatabase()=0;
    virtual void checkSanity();

signals:
    void subscriptionFound(ForumSubscription *sub);
    void subscriptionRemoved(ForumSubscription *sub);
    void databaseStored();
    void subscriptionsChanged();
    void groupListModelChanged(QObject* groupListModel);

private:
    //    GroupListModel m_groupListModel;
};

#endif // FORUMDATABASE_H
