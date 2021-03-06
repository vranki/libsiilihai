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
#include "forumsubscriptionparsed.h"
#include "parserengine.h"
#include "../xmlserialization.h"

ForumSubscriptionParsed::ForumSubscriptionParsed(QObject *parent, bool temp) :
    ForumSubscription(parent, temp, FP_PARSER)
{
    _parser = -1;
}

int ForumSubscriptionParsed::parserId() const {
    return _parser;
}

void ForumSubscriptionParsed::setParserId(int parser) {
    if(parser==_parser) return;
    _parser = parser;
    emit ForumSubscription::changed();
}

void ForumSubscriptionParsed::setParserEngine(ParserEngine *eng) {
    _engine = eng;
    if(parserEngine() && parserEngine()->parser()) {
        setForumUrl(parserEngine()->parser()->forum_url);
        setSupportsLogin(parserEngine()->parser()->supportsLogin());
        setSupportsPosting(_engine->supportsPosting());
    } else {
        emit changed();
    }
}

void ForumSubscriptionParsed::copyFrom(const ForumSubscription *other) {
    ForumSubscription::copyFrom(other);
    const ForumSubscriptionParsed *otherParsed = qobject_cast<ForumSubscriptionParsed const *>(other);
    Q_ASSERT(otherParsed);
    setParserId(otherParsed->parserId());
}

ParserEngine *ForumSubscriptionParsed::parserEngine() const
{
    return qobject_cast<ParserEngine*>(_engine);
}

bool ForumSubscriptionParsed::isSane() const {
    return (_parser > 0 && ForumSubscription::isSane());
}

QUrl ForumSubscriptionParsed::forumUrl() const {
    if(parserEngine() && parserEngine()->parser())
        return QUrl(parserEngine()->parser()->forum_url);
    // Ok, no parser loaded (yet)
    return ForumSubscription::forumUrl();
}

QDomElement ForumSubscriptionParsed::serialize(QDomElement &parent, QDomDocument &doc) {
    QDomElement subElement = ForumSubscription::serialize(parent, doc);
    XmlSerialization::appendValue(SUB_PARSER, QString::number(parserId()), subElement, doc);
    return subElement;
}

void ForumSubscriptionParsed::readSubscriptionXml(QDomElement &element)
{
    ForumSubscription::readSubscriptionXml(element);
    bool ok = false;
    int parser = QString(element.firstChildElement(SUB_PARSER).text()).toInt(&ok);
    setParserId(parser);
}
