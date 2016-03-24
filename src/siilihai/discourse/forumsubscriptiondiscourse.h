#ifndef FORUMSUBSCRIPTIONDISCOURSE_H
#define FORUMSUBSCRIPTIONDISCOURSE_H

#include "../forumdata/forumsubscription.h"

class DiscourseEngine;

class ForumSubscriptionDiscourse : public ForumSubscription
{
    Q_OBJECT

public:
    ForumSubscriptionDiscourse(QObject *parent = 0, bool temp=true);
    void setDiscourseEngine(DiscourseEngine* engine);
};

#endif // FORUMSUBSCRIPTIONDISCOURSE_H
