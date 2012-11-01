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
