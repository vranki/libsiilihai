/* This file is part of libSiilihai.

    libSiilihai is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libSiilihai is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with libSiilihai.  If not, see <http://www.gnu.org/licenses/>. */
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
