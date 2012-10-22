#include "forumsubscriptionparsed.h"
#include "parserengine.h"
#include "../xmlserialization.h"

ForumSubscriptionParsed::ForumSubscriptionParsed(QObject *parent, bool temp) :
    ForumSubscription(parent, temp, FP_PARSER)
{
    _parser = -1;
}

int ForumSubscriptionParsed::parser() const {
    return _parser;
}

void ForumSubscriptionParsed::setParser(int parser) {
    if(parser==_parser) return;
    _parser = parser;
    emit ForumSubscription::changed();
}

void ForumSubscriptionParsed::setParserEngine(ParserEngine *eng) {
    _engine = eng;
    if(parserEngine() && parserEngine()->parser()) {
        setForumUrl(parserEngine()->parser()->forum_url);
    } else {
        emit changed();
    }
}

void ForumSubscriptionParsed::copyFrom(ForumSubscription * other) {
    ForumSubscription::copyFrom(other);
    if(other->isParsed())
        setParser(qobject_cast<ForumSubscriptionParsed*>(other)->parser());
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
    XmlSerialization::appendValue(SUB_PARSER, QString::number(parser()), subElement, doc);
    return subElement;
}

void ForumSubscriptionParsed::readSubscriptionXml(QDomElement &element)
{
    ForumSubscription::readSubscriptionXml(element);
    bool ok = false;
    int parser = QString(element.firstChildElement(SUB_PARSER).text()).toInt(&ok);
    setParser(parser);
}
