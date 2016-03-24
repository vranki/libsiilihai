#include "forumsubscriptiondiscourse.h"
#include "discourseengine.h"

ForumSubscriptionDiscourse::ForumSubscriptionDiscourse(QObject *parent, bool temp):
    ForumSubscription(parent, temp, FP_DISCOURSE)
{
    setSupportsLogin(false);
}

void ForumSubscriptionDiscourse::setDiscourseEngine(DiscourseEngine *engine)
{
    _engine = engine;
    setSupportsPosting(_engine->supportsPosting());
}
