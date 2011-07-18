#include "xmlserialization.h"
#include "forumsubscription.h"
#include "forumgroup.h"
#include "forumthread.h"
#include "forummessage.h"

XmlSerialization::XmlSerialization()
{
}

void XmlSerialization::serialize(ForumSubscription *sub, QDomElement &parent, QDomDocument &doc) {
    QDomElement subElement = doc.createElement(SUB_SUBSCRIPTION);
    appendValue(SUB_PARSER, QString::number(sub->parser()), subElement, doc);
    appendValue(SUB_ALIAS, sub->alias(), subElement, doc);
    appendValue(SUB_USERNAME, sub->username(), subElement, doc);
    appendValue(SUB_PASSWORD, sub->password(), subElement, doc);
    appendValue(SUB_LATEST_THREADS, QString::number(sub->latestThreads()), subElement, doc);
    appendValue(SUB_LATEST_MESSAGES, QString::number(sub->latestMessages()), subElement, doc);

    foreach(ForumGroup *grp, sub->values())
        serialize(grp, subElement, doc);

    parent.appendChild(subElement);
}


void XmlSerialization::serialize(ForumGroup *grp, QDomElement &parent, QDomDocument &doc) {
    QDomElement newElement = doc.createElement(GRP_GROUP);
    appendForumDataItemValues(grp, newElement, doc);
    appendValue(COMMON_CHANGESET, QString::number(grp->changeset()), newElement, doc);
    if(grp->isSubscribed())
        newElement.setAttribute(GRP_SUBSCRIBED, "true");

    foreach(ForumThread *thread, grp->values())
        serialize(thread, newElement, doc);

    parent.appendChild(newElement);
}

void XmlSerialization::serialize(ForumThread *thr, QDomElement &parent, QDomDocument &doc) {
    QDomElement newElement = doc.createElement(THR_THREAD);
    appendForumDataItemValues(thr, newElement, doc);
    appendValue(COMMON_CHANGESET, QString::number(thr->changeset()), newElement, doc);
    appendValue(COMMON_ORDERNUM, QString::number(thr->ordernum()), newElement, doc);
    appendValue(THR_GETMESSAGESCOUNT, QString::number(thr->getMessagesCount()), newElement, doc);
    appendValue(THR_LASTPAGE, QString::number(thr->lastPage()), newElement, doc);

    if(thr->hasMoreMessages())
        newElement.setAttribute(THR_HASMOREMESSAGES, "true");

    foreach(ForumMessage *msg, thr->values())
        serialize(msg, newElement, doc);

    parent.appendChild(newElement);
}

void XmlSerialization::serialize(ForumMessage *msg, QDomElement &parent, QDomDocument &doc) {
    QDomElement newElement = doc.createElement(MSG_MESSAGE);
    appendForumDataItemValues(msg, newElement, doc);
    appendValue(COMMON_ORDERNUM, QString::number(msg->ordernum()), newElement, doc);
    appendValue(MSG_URL, msg->url(), newElement, doc);
    appendValue(MSG_AUTHOR, msg->author(), newElement, doc);
    appendValue(MSG_BODY, msg->body(), newElement, doc);

    if(msg->isRead())
        newElement.setAttribute(MSG_READ, "true");

    parent.appendChild(newElement);
}

void XmlSerialization::appendForumDataItemValues(ForumDataItem *item, QDomElement &parent, QDomDocument &doc) {
    parent.setAttribute(COMMON_ID, item->id());
    appendValue(COMMON_NAME, item->name(), parent, doc);
    appendValue(COMMON_LASTCHANGE, item->lastchange(), parent, doc);
}

void XmlSerialization::appendValue(QString name, QString value, QDomElement &parent, QDomDocument &doc) {
    QDomElement valueElement = doc.createElement(name);
    QDomText textElement = doc.createTextNode(value);
    valueElement.appendChild(textElement);
    parent.appendChild(valueElement);
}
