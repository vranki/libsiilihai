#ifndef XMLSERIALIZATION_H
#define XMLSERIALIZATION_H

class ForumDataItem;
class ForumSubscription;
class ForumGroup;
class ForumThread;
class ForumMessage;
class ForumParser;
class ForumRequest;

#include <QDomElement>
#include <QDomDocument>
#include <QObject>

#define COMMON_ID "id"
#define COMMON_NAME "name"
#define COMMON_LASTCHANGE "lastchange"
#define COMMON_CHANGESET "changeset"
#define COMMON_ORDERNUM "ordernum"

#define SUB_SUBSCRIPTION "subscription"
#define SUB_PARSER "parser"
#define SUB_ALIAS "alias"
#define SUB_USERNAME "username"
#define SUB_PASSWORD "password"
#define SUB_LATEST_THREADS "latest_threads"
#define SUB_LATEST_MESSAGES "latest_messages"

#define GRP_GROUP "group"
#define GRP_SUBSCRIBED "subscribed"

#define THR_THREAD "thread"
#define THR_HASMOREMESSAGES "has_more_messages"
#define THR_GETMESSAGESCOUNT "get_messages_count"
#define THR_LASTPAGE "last_page"

#define MSG_MESSAGE "message"
#define MSG_URL "url"
#define MSG_AUTHOR "author"
#define MSG_BODY "body"
#define MSG_READ "read"

/**
  * Contains static functions for (de)serializing objects as XML
  */
class XmlSerialization
{
public:
    static void serialize(ForumSubscription *sub, QDomElement &parent, QDomDocument &doc);
    static void serialize(ForumGroup *grp, QDomElement &parent, QDomDocument &doc);
    static void serialize(ForumThread *thr, QDomElement &parent, QDomDocument &doc);
    static void serialize(ForumMessage *msg, QDomElement &parent, QDomDocument &doc);
    static ForumSubscription* readSubscription(QDomElement &element, QObject *parent);
    static ForumGroup* readGroup(QDomElement &element, ForumSubscription *parent);
    static ForumThread* readThread(QDomElement &element, ForumGroup *parent);
    static ForumMessage* readMessage(QDomElement &element, ForumThread *parent);

    static void serialize(ForumParser *p, QDomElement &parent, QDomDocument &doc);
    static ForumParser *readParser(QDomElement &element, QObject *parent);

    static ForumRequest *readForumRequest(QDomElement &element, QObject *parent);
private:
    static void appendValue(QString name, QString value, QDomElement &parent, QDomDocument &doc);
    static void appendForumDataItemValues(ForumDataItem *item, QDomElement &parent, QDomDocument &doc);
    static void readForumDataItemValues(ForumDataItem *item, QDomElement &element);
};

#endif // XMLSERIALIZATION_H
