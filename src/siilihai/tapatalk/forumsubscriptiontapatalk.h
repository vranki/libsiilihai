#ifndef TAPATALKFORUMSUBSCRIPTION_H
#define TAPATALKFORUMSUBSCRIPTION_H
#include "../forumdata/forumsubscription.h"
#include <QUrl>

class TapaTalkEngine;
#define SUB_FORUMURL "forumurl"

class ForumSubscriptionTapaTalk : public ForumSubscription
{
    Q_OBJECT
public:
    explicit ForumSubscriptionTapaTalk(QObject *parent = 0, bool temp=true);
    virtual void copyFrom(ForumSubscription * o);
    void setTapaTalkEngine(TapaTalkEngine* newEngine);
    virtual QDomElement serialize(QDomElement &parent, QDomDocument &doc);
    virtual void readSubscriptionXml(QDomElement &element);
};

#endif // TAPATALKFORUMSUBSCRIPTION_H
