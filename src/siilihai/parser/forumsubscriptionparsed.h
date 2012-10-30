#ifndef FORUMSUBSCRIPTIONPARSED_H
#define FORUMSUBSCRIPTIONPARSED_H

#include "../forumdata/forumsubscription.h"

class ParserEngine;
#define SUB_PARSER "parser"
class ForumSubscriptionParsed : public ForumSubscription
{
    Q_OBJECT
//    Q_PROPERTY(int parser READ parser WRITE setParser NOTIFY changed)

public:
    explicit ForumSubscriptionParsed(QObject *parent=0, bool temp=true);
    void setParser(int parser);
    int parser() const;
    void setParserEngine(ParserEngine *eng);
    virtual void copyFrom(ForumSubscription * o);
    ParserEngine *parserEngine() const;
    virtual bool isSane() const;
    virtual QUrl forumUrl() const;
    virtual QDomElement serialize(QDomElement &parent, QDomDocument &doc);
    virtual void readSubscriptionXml(QDomElement &element);

private:
    int _parser;
};

#endif // FORUMSUBSCRIPTIONPARSED_H