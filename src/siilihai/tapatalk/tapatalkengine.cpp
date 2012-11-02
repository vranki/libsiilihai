#include "tapatalkengine.h"
#include "forumsubscriptiontapatalk.h"
#include "../forumdata/forumgroup.h"
#include "../forumdata/forumthread.h"
#include "../forumdata/forummessage.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QByteArray>
#include <QDomDocument>
#include <QString>
#include <QByteArray>
#include <QRegExp>

TapaTalkEngine::TapaTalkEngine(ForumDatabase *fd, QObject *parent) :
    UpdateEngine(parent, fd)
{
    connect(&nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkReply(QNetworkReply*)), Qt::UniqueConnection);
    groupBeingUpdated = 0;
    threadBeingUpdated = 0;
    loggedIn = false;
}

void TapaTalkEngine::setSubscription(ForumSubscription *fs) {
    Q_ASSERT(!fs || qobject_cast<ForumSubscriptionTapaTalk*>(fs));
    UpdateEngine::setSubscription(fs);
    if(!fs) return;
    Q_ASSERT(fs->forumUrl().isValid());
    subscriptionTapaTalk()->setTapaTalkEngine(this);
    connectorUrl = subscriptionTapaTalk()->forumUrl().toString() + "mobiquo/mobiquo.php";
    setState(PES_IDLE);
}


ForumSubscriptionTapaTalk *TapaTalkEngine::subscriptionTapaTalk() const
{
    return qobject_cast<ForumSubscriptionTapaTalk*>(subscription());
}

void TapaTalkEngine::convertBodyToHtml(ForumMessage *msg)
{
    // @todo: this is probably quite stupid way to do this. Use Regexp or something?
    //    QString origBody = msg->body(); // testing
    QString newBody = msg->body();
    newBody = newBody.replace("[/url]", "</a>");
    newBody = newBody.replace("[IMG]", "[img]");
    newBody = newBody.replace("[img]", "<br/><img src=\"");
    newBody = newBody.replace("[/IMG]", "[/img]");
    newBody = newBody.replace("[/img]", "\"><br/>");
    newBody = newBody.replace("[quote]", "<div class=\"quote\">");
    newBody = newBody.replace("[/quote]", "</div><br/>");
    newBody = newBody.replace("\n\n", "<br/>");
    int urlPosition = -1;
    do {
        urlPosition = newBody.indexOf("[url=");
        if(urlPosition >= 0) {
            newBody = newBody.replace(urlPosition, 4, "<a href=\"");
            int closetag = newBody.indexOf("]", urlPosition);
            if(closetag >= 0) {
                newBody = newBody.replace(closetag, 1, "\">");
            }
        }
    } while(urlPosition >= 0);

    //    newBody.append("-----" + origBody);
    msg->setBody(newBody);
}

void TapaTalkEngine::probeUrl(QUrl url)
{
    connectorUrl = url.toString() + "mobiquo/mobiquo.php";
    qDebug() << Q_FUNC_INFO << "will now probe " << connectorUrl;
    QNetworkRequest req(connectorUrl);
    QDomDocument doc("");
    QDomElement root = doc.createElement("methodCall");
    doc.appendChild(root);

    QDomElement methodTag = doc.createElement("methodName");
    root.appendChild(methodTag);

    QDomText t = doc.createTextNode("get_config");
    methodTag.appendChild(t);

    QByteArray requestData = doc.toByteArray();
    req.setAttribute(QNetworkRequest::User, TTO_Probe);
    nam.post(req, requestData);
}

void TapaTalkEngine::replyProbe(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit urlProbeResults(0);
        return;
    }
    QString docs = QString().fromUtf8(reply->readAll());

    QDomDocument doc;
    doc.setContent(docs);
    QDomElement arrayDataElement = doc.firstChildElement("methodResponse");
    if(!arrayDataElement.isNull()) {
        ForumSubscription sub(0, true, ForumSubscription::FP_TAPATALK);
        emit urlProbeResults(&sub);
    } else {
        emit urlProbeResults(0);
    }
}

void TapaTalkEngine::requestCredentials() {
    UpdateEngine::requestCredentials();
    emit getForumAuthentication(subscription());
}

void TapaTalkEngine::doUpdateForum() {
    QNetworkRequest req(connectorUrl);
    QDomDocument doc("");
    QDomElement root = doc.createElement("methodCall");
    doc.appendChild(root);

    QDomElement methodTag = doc.createElement("methodName");
    root.appendChild(methodTag);

    QDomText t = doc.createTextNode("get_forum");
    methodTag.appendChild(t);

    QByteArray requestData = doc.toByteArray();
    req.setAttribute(QNetworkRequest::User, TTO_ListGroups);
    nam.post(req, requestData);
}

void TapaTalkEngine::doUpdateGroup(ForumGroup *group)
{
    Q_ASSERT(!groupBeingUpdated);
    groupBeingUpdated = group;
    QNetworkRequest req(connectorUrl);
    QDomDocument doc("");
    QDomElement methodCallElement = doc.createElement("methodCall");
    doc.appendChild(methodCallElement);

    QDomElement methodTag = doc.createElement("methodName");
    methodCallElement.appendChild(methodTag);

    QDomText t = doc.createTextNode("get_topic");
    methodTag.appendChild(t);

    QDomElement paramsTag = doc.createElement("params");
    methodCallElement.appendChild(paramsTag);

    QDomElement paramTag = doc.createElement("param");
    paramsTag.appendChild(paramTag);

    QDomElement forumIdValueTag = doc.createElement("value");
    paramTag.appendChild(forumIdValueTag);

    QDomElement forumIdStringTag = doc.createElement("string");
    forumIdValueTag.appendChild(forumIdStringTag);

    QDomText forumIdValueText = doc.createTextNode(group->id());
    forumIdStringTag.appendChild(forumIdValueText);

    QByteArray requestData = doc.toByteArray();
    req.setAttribute(QNetworkRequest::User, TTO_UpdateGroup);
    nam.post(req, requestData);
}

void TapaTalkEngine::replyUpdateGroup(QNetworkReply *reply)
{
    Q_ASSERT(groupBeingUpdated);
    QList<ForumThread*> threads;
    if (reply->error() != QNetworkReply::NoError) {
        listThreadsFinished(threads, groupBeingUpdated);
        groupBeingUpdated = 0;
        return;
    }
    QString docs = QString().fromUtf8(reply->readAll());
    QDomDocument doc;
    doc.setContent(docs);

    QDomElement paramValueElement = doc.firstChildElement("methodResponse").firstChildElement("params").firstChildElement("param").firstChildElement("value");
    QDomElement topicsValueElement = findMemberValueElement(paramValueElement, "topics");

    getThreads(topicsValueElement.firstChildElement("array").firstChildElement("data"), &threads);
    listThreadsFinished(threads, groupBeingUpdated);
    qDeleteAll(threads);
    groupBeingUpdated = 0;
}


void TapaTalkEngine::getThreads(QDomElement arrayDataElement, QList<ForumThread *> *threads)
{
    Q_ASSERT(arrayDataElement.nodeName()=="data");
    QDomElement dataValueElement = arrayDataElement.firstChildElement("value");
    while(!dataValueElement.isNull()) {
        QString name = getValueFromStruct(dataValueElement, "topic_title");
        QString id = getValueFromStruct(dataValueElement, "topic_id");
        QString author = getValueFromStruct(dataValueElement, "topic_author_name");
        QString lc = getValueFromStruct(dataValueElement, "last_reply_time");

        if(!id.isNull()) {
            ForumThread *newThread = new ForumThread(groupBeingUpdated, true);
            newThread->setId(id);
            newThread->setName(name);
            newThread->setLastchange(lc);
            newThread->setGetMessagesCount(subscription()->latestMessages());
            newThread->setOrdernum(threads->size());

            threads->append(newThread);
        }

        dataValueElement = dataValueElement.nextSiblingElement();
    }
}

void TapaTalkEngine::doUpdateThread(ForumThread *thread)
{
    Q_ASSERT(!threadBeingUpdated);
    threadBeingUpdated = thread;

    QNetworkRequest req(connectorUrl);
    QDomDocument doc("");
    QDomElement methodCallElement = doc.createElement("methodCall");
    doc.appendChild(methodCallElement);

    QDomElement methodTag = doc.createElement("methodName");
    methodCallElement.appendChild(methodTag);

    QDomText t = doc.createTextNode("get_thread");
    methodTag.appendChild(t);

    QDomElement paramsTag = doc.createElement("params");
    methodCallElement.appendChild(paramsTag);

    QDomElement paramTag = doc.createElement("param");
    paramsTag.appendChild(paramTag);

    QDomElement forumIdValueTag = doc.createElement("value");
    paramTag.appendChild(forumIdValueTag);

    QDomElement forumIdStringTag = doc.createElement("string");
    forumIdValueTag.appendChild(forumIdStringTag);

    QDomText forumIdValueText = doc.createTextNode(thread->id());
    forumIdStringTag.appendChild(forumIdValueText);

    QByteArray requestData = doc.toByteArray();
    req.setAttribute(QNetworkRequest::User, TTO_UpdateThread);
    nam.post(req, requestData);
}

void TapaTalkEngine::replyUpdateThread(QNetworkReply *reply)
{
    QList<ForumMessage*> messages;
    if (reply->error() != QNetworkReply::NoError) {
        listMessagesFinished(messages, threadBeingUpdated, false);
        return;
    }

    QString docs = QString().fromUtf8(reply->readAll());
    QDomDocument doc;
    doc.setContent(docs);
    QDomElement paramValueElement = doc.firstChildElement("methodResponse").firstChildElement("params").firstChildElement("param").firstChildElement("value");
    getMessages(paramValueElement, &messages);
    ForumThread *updatedThread = threadBeingUpdated;
    threadBeingUpdated=0;
    listMessagesFinished(messages, updatedThread, false);
    qDeleteAll(messages);
}

bool TapaTalkEngine::loginIfNeeded() {
    if(loggedIn) return false;
    if(!subscription()->authenticated()) return false;

    QNetworkRequest req(connectorUrl);
    QDomDocument doc("");
    QDomElement methodCallElement = doc.createElement("methodCall");
    doc.appendChild(methodCallElement);

    QDomElement methodTag = doc.createElement("methodName");
    methodCallElement.appendChild(methodTag);

    QDomText t = doc.createTextNode("login");
    methodTag.appendChild(t);

    QDomElement paramsTag = doc.createElement("params");
    methodCallElement.appendChild(paramsTag);

    QDomElement paramTag = doc.createElement("param");
    paramsTag.appendChild(paramTag);

    QDomElement usernameValueTag = doc.createElement("value");
    paramTag.appendChild(usernameValueTag);

    QDomElement usernameStringTag = doc.createElement("string");
    usernameValueTag.appendChild(usernameStringTag);

    QDomText usernameValueText = doc.createTextNode(subscription()->username());
    usernameStringTag.appendChild(usernameValueText);

    QDomElement passwordParamTag = doc.createElement("param");
    paramsTag.appendChild(passwordParamTag);

    QDomElement passwordValueTag = doc.createElement("value");
    passwordParamTag.appendChild(passwordValueTag);

    QDomElement passwordStringTag = doc.createElement("string");
    passwordValueTag.appendChild(passwordStringTag);

    QDomText passwordValueText = doc.createTextNode(subscription()->password());
    passwordStringTag.appendChild(passwordValueText);

    QByteArray requestData = doc.toByteArray();
    req.setAttribute(QNetworkRequest::User, TTO_Login);
    nam.post(req, requestData);

    return true;
}

void TapaTalkEngine::replyLogin(QNetworkReply *reply)
{
}

void TapaTalkEngine::getMessages(QDomElement dataValueElement, QList<ForumMessage *> *messages)
{
    Q_ASSERT(dataValueElement.nodeName()=="value");
    QDomElement postsValueElement = findMemberValueElement(dataValueElement, "posts");
    QDomElement arrayDataValueElement = postsValueElement.firstChildElement("array").firstChildElement("data").firstChildElement("value");
    while(!arrayDataValueElement.isNull()) {
        QString id = getValueFromStruct(arrayDataValueElement, "post_id");
        if(!id.isNull()) {
            ForumMessage *newMessage = new ForumMessage(threadBeingUpdated, true);
            newMessage->setId(id);
            newMessage->setAuthor(getValueFromStruct(arrayDataValueElement, "post_author_name"));
            newMessage->setName(getValueFromStruct(arrayDataValueElement, "post_title"));
            newMessage->setBody(getValueFromStruct(arrayDataValueElement, "post_content"));
            newMessage->setLastchange(getValueFromStruct(arrayDataValueElement, "post_time"));
            newMessage->setOrdernum(messages->size());
            newMessage->setRead(false, false);
            convertBodyToHtml(newMessage);
            messages->append(newMessage);
        }
        arrayDataValueElement = arrayDataValueElement.nextSiblingElement();
    }
}


void TapaTalkEngine::networkReply(QNetworkReply *reply)
{
    int operationAttribute = reply->request().attribute(QNetworkRequest::User).toInt();
    if(!operationAttribute) {
        qDebug( ) << Q_FUNC_INFO << "Reply " << operationAttribute << " not for me";
        return;
    }
    if(operationAttribute==TTO_None) {
        Q_ASSERT(false);
    } else if(operationAttribute==TTO_ListGroups) {
        replyListGroups(reply);
    } else if(operationAttribute==TTO_UpdateGroup) {
        replyUpdateGroup(reply);
    } else if(operationAttribute==TTO_UpdateThread) {
        replyUpdateThread(reply);
    } else if(operationAttribute==TTO_Probe) {
        replyProbe(reply);
    }  else if(operationAttribute==TTO_Login) {
        replyLogin(reply);
    } else {
        Q_ASSERT(false);
    }
}


void TapaTalkEngine::replyListGroups(QNetworkReply *reply)
{
    QList<ForumGroup*> grps;
    if (reply->error() != QNetworkReply::NoError) {
        listGroupsFinished(grps);
        return;
    }
    QString docs = QString().fromUtf8(reply->readAll());
    QDomDocument doc;
    doc.setContent(docs);
    QDomElement arrayDataElement = doc.firstChildElement("methodResponse").firstChildElement("params").firstChildElement("param").firstChildElement("value").firstChildElement("array").firstChildElement("data");
    getGroups(arrayDataElement, &grps);
    listGroupsFinished(grps);
    qDeleteAll(grps);
}


void TapaTalkEngine::getGroups(QDomElement arrayDataElement, QList<ForumGroup *> *grps) {
    Q_ASSERT(arrayDataElement.nodeName()=="data");
    QDomElement arrayValueElement = arrayDataElement.firstChildElement("value");
    while(!arrayValueElement.isNull()) {
        QString groupId = getValueFromStruct(arrayValueElement, "forum_id");
        QString groupName = getValueFromStruct(arrayValueElement, "forum_name");
        QString subOnly = getValueFromStruct(arrayValueElement, "sub_only");
        QString newPosts = getValueFromStruct(arrayValueElement, "new_post");

        if(subOnly=="0") { // Add only leaf groups
            ForumGroup *newGroup = new ForumGroup(subscription(), true);
            newGroup->setName(groupName);
            newGroup->setId(groupId);
            newGroup->setChangeset(rand()); // Testing
            if(newPosts=="1") { // Mark as changed
                newGroup->setChangeset(rand());
            }
            grps->append(newGroup);
        }
        getChildGroups(arrayValueElement, grps);
        arrayValueElement = arrayValueElement.nextSiblingElement();
    }
}

QString TapaTalkEngine::getValueFromStruct(QDomElement arrayValueElement, QString name) {
    Q_ASSERT(arrayValueElement.nodeName()=="value");

    QDomElement valueElement = findMemberValueElement(arrayValueElement, name);
    if(!valueElement.isNull()) {
        QDomElement valueTypeElement = valueElement.firstChildElement();
        if(valueTypeElement.nodeName()=="string") {
            return valueTypeElement.text();
        } else if(valueTypeElement.nodeName()=="base64") {
            QByteArray valueText = QByteArray::fromBase64(valueTypeElement.text().toUtf8());
            return valueText.data();
        } else if(valueTypeElement.nodeName()=="boolean") {
            return valueTypeElement.text();
        }
    }
    return QString();
}

void TapaTalkEngine::getChildGroups(QDomElement arrayValueElement, QList<ForumGroup *> *grps)
{
    Q_ASSERT(arrayValueElement.nodeName()=="value");
    QDomElement valueElement = findMemberValueElement(arrayValueElement, "child");
    if(!valueElement.isNull()) {
        QDomElement dataElement = valueElement.firstChildElement("array").firstChildElement("data");
        if(!dataElement.isNull()) {
            getGroups(dataElement, grps);
        }
    }
}


QDomElement TapaTalkEngine::findMemberValueElement(QDomElement arrayValueElement, QString memberName)
{
    Q_ASSERT(arrayValueElement.nodeName()=="value");
    QDomElement structElement = arrayValueElement.firstChildElement("struct");
    QDomElement memberElement = structElement.firstChildElement("member");
    while(!memberElement.isNull()) {
        QDomElement nameElement = memberElement.firstChildElement("name");
        if(nameElement.text() == memberName) {
            QDomElement valueElement = memberElement.firstChildElement("value");
            return valueElement;
        }
        memberElement = memberElement.nextSiblingElement();
    }
    return QDomElement();
}
