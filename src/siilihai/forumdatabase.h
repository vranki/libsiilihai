#ifndef FORUMDATABASE_H
#define FORUMDATABASE_H

#include <QObject>
#include <QMap>

class ForumSubscription;
class ForumGroup;
class ForumThread;
class ForumMessage;

class ForumDatabase : public QObject, public QMap<int, ForumSubscription*> {
    Q_OBJECT
public:
    explicit ForumDatabase(QObject *parent = 0);
    virtual void resetDatabase()=0;
    virtual int schemaVersion()=0;
    virtual bool isStored()=0;

    // Subscription related
    virtual bool addSubscription(ForumSubscription *fs)=0; // Ownership changes!!!
    virtual void deleteSubscription(ForumSubscription *sub)=0;

    // Thread related
    ForumThread* getThread(const int forum, QString groupid, QString threadid);

    // Message related
    ForumMessage* getMessage(const int forum, QString groupid, QString threadid, QString messageid);

public slots:
    virtual bool storeDatabase()=0;
    virtual void checkSanity();

signals:
    void subscriptionFound(ForumSubscription *sub);
    void subscriptionRemoved(ForumSubscription *sub);
    void databaseStored();
};

#endif // FORUMDATABASE_H
