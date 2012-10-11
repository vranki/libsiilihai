#include "forumsubscriptiontapatalk.h"
#include "tapatalkengine.h"

ForumSubscriptionTapaTalk::ForumSubscriptionTapaTalk(QObject *parent, bool temp) :
    ForumSubscription(parent, temp, FP_TAPATALK)
{
}

void ForumSubscriptionTapaTalk::copyFrom(ForumSubscription * other) {
    ForumSubscription::copyFrom(other);
    if(other->isTapaTalk())
        setForumUrl(qobject_cast<ForumSubscriptionTapaTalk*>(other)->forumUrl());
}

void ForumSubscriptionTapaTalk::setForumUrl(QUrl url)
{
    _forumUrl = url;
    emit ForumSubscription::changed();
}

QUrl ForumSubscriptionTapaTalk::forumUrl() const
{
    return _forumUrl;
}

void ForumSubscriptionTapaTalk::setTapaTalkEngine(TapaTalkEngine *newEngine)
{
    _engine = newEngine;
}
