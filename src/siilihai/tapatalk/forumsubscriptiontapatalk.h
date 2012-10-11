#ifndef TAPATALKFORUMSUBSCRIPTION_H
#define TAPATALKFORUMSUBSCRIPTION_H
#include "../forumdata/forumsubscription.h"
#include <QUrl>

class TapaTalkEngine;

class ForumSubscriptionTapaTalk : public ForumSubscription
{
    Q_OBJECT
public:
    explicit ForumSubscriptionTapaTalk(QObject *parent = 0, bool temp=true);
    virtual void copyFrom(ForumSubscription * o);
    void setForumUrl(QUrl url);
    QUrl forumUrl() const;
    void setTapaTalkEngine(TapaTalkEngine* newEngine);
signals:
    
public slots:
private:
    QUrl _forumUrl;
};

#endif // TAPATALKFORUMSUBSCRIPTION_H
