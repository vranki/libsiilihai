#include "forumsubscriptionparsed.h"
#include "parserengine.h"

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
    emit ForumSubscription::changed();
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
    return QUrl();
}
