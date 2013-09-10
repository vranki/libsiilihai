#include "xmlserialization.h"
#include "forumdata/forumsubscription.h"
#include "forumdata/forumgroup.h"
#include "forumdata/forumthread.h"
#include "forumdata/forummessage.h"
#include "parser/forumparser.h"
#include "parser/forumsubscriptionparsed.h"
#include "tapatalk/forumsubscriptiontapatalk.h"
#include "forumrequest.h"
#include <QDate>

void XmlSerialization::serialize(ForumSubscription *sub, QDomElement &parent, QDomDocument &doc) {
    QDomElement subElement = sub->serialize(parent, doc);
    foreach(ForumGroup *grp, sub->values())
        serialize(grp, subElement, doc);
}

void XmlSerialization::serialize(ForumGroup *grp, QDomElement &parent, QDomDocument &doc) {
    QDomElement newElement = doc.createElement(GRP_GROUP);
    appendForumDataItemValues(grp, newElement, doc);
    appendValue(COMMON_CHANGESET, QString::number(grp->changeset()), newElement, doc);
    appendValue(GRP_HIERARCHY, grp->hierarchy(), newElement, doc);
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

ForumSubscription *XmlSerialization::readSubscription(QDomElement &element, QObject *parent)
{
    ForumSubscription *sub = ForumSubscription::readSubscription(element, parent);
    if(sub) {
        QDomElement groupElement = element.firstChildElement(GRP_GROUP);
        while(!groupElement.isNull()) {
            ForumGroup *grp = readGroup(groupElement, sub);
            if(grp) {
                sub->addGroup(grp, false, false);
                if(grp->needsToBeUpdated())
                    sub->markToBeUpdated(true);
            }
            groupElement = groupElement.nextSiblingElement(GRP_GROUP);
        }
    }
    return sub;
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


ForumGroup* XmlSerialization::readGroup(QDomElement &element, ForumSubscription *parent) {
    if(element.tagName() != GRP_GROUP) return 0;

    ForumGroup *grp = new ForumGroup(parent, false);
    readForumDataItemValues(grp, element);
    grp->setSubscribed(!element.attribute(GRP_SUBSCRIBED).isNull());
    grp->setChangeset(QString(element.firstChildElement(COMMON_CHANGESET).text()).toInt());
    grp->setHierarchy(element.attribute(GRP_HIERARCHY));
    if(grp->name()==UNKNOWN_SUBJECT) grp->markToBeUpdated();
    if(grp->isSubscribed()) {
        QDomElement threadElement = element.firstChildElement(THR_THREAD);
        while(!threadElement.isNull()) {
            ForumThread *thr = readThread(threadElement, grp);
            if(thr) {
                grp->addThread(thr, false, false);
                if(thr->needsToBeUpdated())
                    grp->markToBeUpdated(true);
            }
            threadElement = threadElement.nextSiblingElement(THR_THREAD);
        }
        if(grp->isEmpty()) // Force update if contains no threads
            grp->markToBeUpdated();
    }

    return grp;
}

ForumThread* XmlSerialization::readThread(QDomElement &element, ForumGroup *parent) {
    if(element.tagName() != THR_THREAD) return 0;

    ForumThread *thr = new ForumThread(parent, false);
    readForumDataItemValues(thr, element);
    thr->setHasMoreMessages(!element.attribute(THR_HASMOREMESSAGES).isNull());
    thr->setChangeset(element.firstChildElement(COMMON_CHANGESET).text().toInt());
    thr->setOrdernum(element.firstChildElement(COMMON_ORDERNUM).text().toInt());
    thr->setGetMessagesCount(element.firstChildElement(THR_GETMESSAGESCOUNT).text().toInt());
    thr->setLastPage(element.firstChildElement(THR_LASTPAGE).text().toInt());

    if(thr->name()==UNKNOWN_SUBJECT) thr->markToBeUpdated();
    if(thr->needsToBeUpdated()) parent->markToBeUpdated();

    QDomElement messageElement = element.firstChildElement(MSG_MESSAGE);
    while(!messageElement.isNull()) {
        ForumMessage *msg = readMessage(messageElement, thr);
        if(msg) {
            thr->addMessage(msg, false);
            if(msg->needsToBeUpdated())
                thr->markToBeUpdated(true);
        }
        messageElement = messageElement.nextSiblingElement(MSG_MESSAGE);
    }
    if(thr->isEmpty()) // Force update if contains no messages
        thr->markToBeUpdated();

    return thr;
}

ForumMessage* XmlSerialization::readMessage(QDomElement &element, ForumThread *parent) {
    if(element.tagName() != MSG_MESSAGE) return 0;

    ForumMessage *msg = new ForumMessage(parent, false);
    readForumDataItemValues(msg, element);
    msg->setRead(!element.attribute(MSG_READ).isNull(), false);
    msg->setOrdernum(element.firstChildElement(COMMON_ORDERNUM).text().toInt());
    msg->setUrl(element.firstChildElement(MSG_URL).text());
    msg->setAuthor(element.firstChildElement(MSG_AUTHOR).text());
    msg->setBody(element.firstChildElement(MSG_BODY).text());
    if(msg->name()==UNKNOWN_SUBJECT) msg->markToBeUpdated();

    return msg;
}

void XmlSerialization::readForumDataItemValues(ForumDataItem *item, QDomElement &element) {
    item->setId(element.attribute(COMMON_ID));
    item->setName(element.firstChildElement(COMMON_NAME).text());
    item->setLastchange(element.firstChildElement(COMMON_LASTCHANGE).text());
}

ForumParser *XmlSerialization::readParser(QDomElement &element, QObject *parent) {
    if(element.tagName() != "parser") return 0;
    ForumParser *parser = new ForumParser(parent);

    parser->setId(element.firstChildElement("id").text().toInt());
    parser->setName(element.firstChildElement("name").text());
    parser->forum_url = element.firstChildElement("forum_url").text();
    parser->parser_status = element.firstChildElement("status").text().toInt();
    parser->thread_list_path = element.firstChildElement("thread_list_path").text();
    parser->view_thread_path = element.firstChildElement("view_thread_path").text();
    parser->login_path = element.firstChildElement("login_path").text();
    parser->date_format = element.firstChildElement("date_format").text().toInt();
    parser->group_list_pattern = element.firstChildElement("group_list_pattern").text();
    parser->thread_list_pattern = element.firstChildElement("thread_list_pattern").text();
    parser->message_list_pattern = element.firstChildElement("message_list_pattern").text();
    parser->verify_login_pattern = element.firstChildElement("verify_login_pattern").text();
    parser->login_parameters = element.firstChildElement("login_parameters").text();
    parser->login_type = (ForumParser::ForumLoginType) element.firstChildElement("login_type").text().toInt();
    parser->charset = element.firstChildElement("charset").text().toLower();
    parser->thread_list_page_start = element.firstChildElement("thread_list_page_start").text().toInt();
    parser->thread_list_page_increment = element.firstChildElement("thread_list_page_increment").text().toInt();
    parser->view_thread_page_start = element.firstChildElement("view_thread_page_start").text().toInt();
    parser->view_thread_page_increment = element.firstChildElement("view_thread_page_increment").text().toInt();
    parser->forum_software = element.firstChildElement("forum_software").text();
    parser->view_message_path = element.firstChildElement("view_message_path").text();
    parser->parser_type = element.firstChildElement("parser_type").text().toInt();
    parser->posting_path = element.firstChildElement("posting_path").text();
    parser->posting_subject = element.firstChildElement("posting_subject").text();
    parser->posting_message = element.firstChildElement("posting_message").text();
    parser->posting_parameters = element.firstChildElement("posting_parameters").text();
    parser->posting_hints = element.firstChildElement("posting_hints").text();
    QString ud = element.firstChildElement("update_date").text();
    if(ud.isNull()) {
        parser->update_date = QDate(1970, 1, 1);
    } else {
        parser->update_date = QDate::fromString(ud);
    }
    return parser;
}

void XmlSerialization::serialize(ForumParser *p, QDomElement &parent, QDomDocument &doc) {
    QDomElement newElement = doc.createElement("parser");
    appendValue("id", QString::number(p->id()), newElement, doc);
    appendValue("name", p->name(), newElement, doc);
    appendValue("forum_url", p->forum_url, newElement, doc);
    appendValue("status", QString::number(p->parser_status), newElement, doc);
    appendValue("thread_list_path", p->thread_list_path, newElement, doc);
    appendValue("view_thread_path", p->view_thread_path, newElement, doc);
    appendValue("login_path", p->login_path, newElement, doc);
    appendValue("date_format", QString::number(p->date_format), newElement, doc);
    appendValue("group_list_pattern", p->group_list_pattern, newElement, doc);
    appendValue("thread_list_pattern", p->thread_list_pattern, newElement, doc);
    appendValue("message_list_pattern", p->message_list_pattern, newElement, doc);
    appendValue("verify_login_pattern", p->verify_login_pattern, newElement, doc);
    appendValue("login_parameters", p->login_parameters, newElement, doc);
    appendValue("login_type", QString::number(p->login_type), newElement, doc);
    appendValue("charset", p->charset, newElement, doc);
    appendValue("thread_list_page_start", QString::number(p->thread_list_page_start), newElement, doc);
    appendValue("thread_list_page_increment", QString::number(p->thread_list_page_increment), newElement, doc);
    appendValue("view_thread_page_start", QString::number(p->view_thread_page_start), newElement, doc);
    appendValue("view_thread_page_increment", QString::number(p->view_thread_page_increment), newElement, doc);
    appendValue("forum_software", p->forum_software, newElement, doc);
    appendValue("view_message_path", p->view_message_path, newElement, doc);
    appendValue("parser_type", QString::number(p->parser_type), newElement, doc);
    appendValue("posting_path", p->posting_path, newElement, doc);
    appendValue("posting_subject", p->posting_subject, newElement, doc);
    appendValue("posting_message", p->posting_message, newElement, doc);
    appendValue("posting_parameters", p->posting_parameters, newElement, doc);
    appendValue("posting_hints", p->posting_hints, newElement, doc);
    appendValue("update_date", p->update_date.toString(), newElement, doc);
    parent.appendChild(newElement);
}

ForumRequest *XmlSerialization::readForumRequest(QDomElement &element, QObject *parent) {
    if(element.tagName() != "request") return 0;

    ForumRequest *request = new ForumRequest(parent);
    request->forum_url = element.firstChildElement("forum_url").text();
    request->comment = element.firstChildElement("comment").text();
    request->date = element.firstChildElement("date").text();
    request->user = element.firstChildElement("user").text();

    return request;
}
