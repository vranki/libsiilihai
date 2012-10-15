#include "forumsubscriptiontapatalk.h"
#include "tapatalkengine.h"
#include "../xmlserialization.h"

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


QDomElement ForumSubscriptionTapaTalk::serialize(QDomElement &parent, QDomDocument &doc) {
    QDomElement subElement = ForumSubscription::serialize(parent, doc);
    XmlSerialization::appendValue(SUB_FORUMURL, forumUrl().toString(), subElement, doc);
    return subElement;
}

void ForumSubscriptionTapaTalk::readSubscriptionXml(QDomElement &element)
{
    ForumSubscription::readSubscriptionXml(element);

    QUrl forumUrl = QUrl(element.firstChildElement(SUB_FORUMURL).text());
    Q_ASSERT(forumUrl.isValid());
    setForumUrl(forumUrl);
}
