#ifndef TAPATALKENGINE_H
#define TAPATALKENGINE_H
#include "../updateengine.h"

class ForumSubscriptionTapaTalk;

class TapaTalkEngine : public UpdateEngine
{
    Q_OBJECT

    enum TapaTalkOperation {
        TTO_None=0, TTO_ListGroups
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

    ForumSubscriptionTapaTalk *subscriptionTapaTalk() const;
};

#endif // TAPATALKENGINE_H
