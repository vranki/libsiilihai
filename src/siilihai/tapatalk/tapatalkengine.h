#ifndef TAPATALKENGINE_H
#define TAPATALKENGINE_H
#include "../updateengine.h"

class ForumSubscriptionTapaTalk;
class QDomElement;

class TapaTalkEngine : public UpdateEngine
{
    Q_OBJECT

    enum TapaTalkOperation {
        TTO_None=0, TTO_ListGroups, TTO_UpdateGroup, TTO_UpdateThread
    };

public:
    explicit TapaTalkEngine(ForumDatabase *fd, QObject *parent);
    virtual void setSubscription(ForumSubscription *fs);
signals:
    
public slots:
protected:
    //    virtual void requestCredentials();
    virtual void doUpdateForum();
    virtual void doUpdateGroup(ForumGroup *group);
    virtual void doUpdateThread(ForumThread *thread);
private slots:
    void networkReply(QNetworkReply *reply);
private:
    void replyListGroups(QNetworkReply *reply);
    void replyUpdateGroup(QNetworkReply *reply);
    void replyUpdateThread(QNetworkReply *reply);
    QString getValueFromStruct(QDomElement dataValueElement, QString name);
    void getGroups(QDomElement dataValueElement, QList<ForumGroup *> *grps);
    void getChildGroups(QDomElement dataValueElement, QList<ForumGroup *> *grps);

    void getThreads(QDomElement dataValueElement, QList<ForumThread *> *threads);
    void getMessages(QDomElement dataValueElement, QList<ForumMessage *> *messages);
    QDomElement findMemberValueElement(QDomElement dataValueElement, QString memberName);
    ForumSubscriptionTapaTalk *subscriptionTapaTalk() const;
    QString connectorUrl;
    ForumGroup *groupBeingUpdated;
    ForumThread *threadBeingUpdated;
};

#endif // TAPATALKENGINE_H
