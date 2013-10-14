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
    setState(UES_IDLE);
}


ForumSubscriptionTapaTalk *TapaTalkEngine::subscriptionTapaTalk() const
{
    return qobject_cast<ForumSubscriptionTapaTalk*>(subscription());
}

void TapaTalkEngine::convertBodyToHtml(ForumMessage *msg)
{
    // @todo: this is probably quite stupid way to do this. Use Regexp or something?

    // @todo replace quotes like this [quote name='GlobalGentleman' timestamp='1355152101' post='2541234']

    QString origBody = msg->body();
    QString newBody = origBody;
    newBody = newBody.replace("[/url]", "</a>");
    newBody = newBody.replace("[/URL]", "</a>");
    newBody = newBody.replace("[URL]", "[url]");
    newBody = newBody.replace("[URL=", "[url=");
    newBody = newBody.replace("[IMG]", "[img]");
    newBody = newBody.replace("[img]", "<br/><img src=\"");
    newBody = newBody.replace("[/IMG]", "[/img]");
    newBody = newBody.replace("[/img]", "\"><br/>");
    newBody = newBody.replace("[quote]", "<div class=\"quote\">");
    newBody = newBody.replace("[/quote]", "</div><br/>");
    newBody = newBody.replace("[QUOTE]", "<div class=\"quote\">");
    newBody = newBody.replace("[/QUOTE]", "</div><br/>");
    newBody = newBody.replace("[color=Black]", "");
    newBody = newBody.replace("[/color]", "");
    newBody = newBody.replace("[DIV]", "<div>");
    newBody = newBody.replace("[/DIV]", "</div>");
    newBody = newBody.replace("Â", ""); // WTF is this character doing in some posts.
    newBody = newBody.replace("\n", "<br/>");

    // Replace [url=]
    // @todo do more smartly
    int urlPosition = -1;
    do {
        urlPosition = newBody.indexOf("[url=");
        if(urlPosition >= 0) {
            newBody = newBody.replace(urlPosition, 5, "<a href=\"");
            int closetag = newBody.indexOf("]", urlPosition);
            if(closetag >= 0) {
                newBody = newBody.replace(closetag, 1, "\">");
            }
        }
    } while(urlPosition >= 0);

    // Replace [url]http://foobar</a> with <a href="http://foobar">http://foobar</a>
    urlPosition = -1;
    do {
        urlPosition = newBody.indexOf("[url]");
        if(urlPosition >= 0) {
            int urlEndPosition = newBody.indexOf("</a>", urlPosition);
            if(urlEndPosition >= urlPosition) {
                QString urlAddress = newBody.mid(urlPosition + 5, urlEndPosition - urlPosition - 5);
                urlAddress = "<a href=\"" + urlAddress + "\">";
                newBody = newBody.replace(urlPosition, 5, urlAddress);
            }
        }
    } while(urlPosition >= 0);

    // newBody.append("----- <pre>" + origBody + "</pre>");
    msg->setBody(newBody);
}

void TapaTalkEngine::probeUrl(QUrl url)
{
    connectorUrl = url.toString() + "mobiquo/mobiquo.php";
    qDebug() << Q_FUNC_INFO << "will now probe " << connectorUrl;
    QNetworkRequest req(connectorUrl);
    QList<QPair<QString, QString> > params;
    QDomDocument doc("");
    createMethodCall(doc, "get_config", params);

    QByteArray requestData = doc.toByteArray();
    req.setHeader(QNetworkRequest::ContentTypeHeader, QString("application/x-www-form-urlencoded"));
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
    if(loginIfNeeded()) return;

    QNetworkRequest req(connectorUrl);

    QList<QPair<QString, QString> > params;
    QDomDocument doc("");
    createMethodCall(doc, "get_forum", params);

    QByteArray requestData = doc.toByteArray();
    req.setAttribute(QNetworkRequest::User, TTO_ListGroups);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QString("application/x-www-form-urlencoded"));
    nam.post(req, requestData);
}

void TapaTalkEngine::doUpdateGroup(ForumGroup *group)
{
    if(loginIfNeeded()) return;

    Q_ASSERT(!groupBeingUpdated);

    groupBeingUpdated = group;
    QNetworkRequest req(connectorUrl);

    QList<QPair<QString, QString> > params;
    params.append(QPair<QString, QString>("string", group->id()));

    QDomDocument doc("");

    createMethodCall(doc, "get_topic", params);

    QByteArray requestData = doc.toByteArray();
    req.setAttribute(QNetworkRequest::User, TTO_UpdateGroup);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QString("application/x-www-form-urlencoded"));
    nam.post(req, requestData);
}

void TapaTalkEngine::replyUpdateGroup(QNetworkReply *reply) {
    if(!groupBeingUpdated) {
        qDebug() << Q_FUNC_INFO << "We are not updating group currently - ignoring this";
        return;
    }
    Q_ASSERT(groupBeingUpdated);
    QList<ForumThread*> threads;
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << Q_FUNC_INFO << reply->errorString() << QString().fromUtf8(reply->readAll());
        emit updateFailure(subscription(), "Error while updating group " + groupBeingUpdated->name() + "\nUnexpected TapatTalk reply.\nThis is a bug in Siilihai or TapaTalk server.\n" + reply->errorString());
        ForumGroup *updatedGroup = groupBeingUpdated;
        groupBeingUpdated = 0; // Update finished so clear this.
        listThreadsFinished(threads, updatedGroup);
        return;
    }
    QString docs = QString().fromUtf8(reply->readAll());
    QDomDocument doc;
    doc.setContent(docs);

    QDomElement paramValueElement = doc.firstChildElement("methodResponse").firstChildElement("params").firstChildElement("param").firstChildElement("value");
    QDomElement topicsValueElement = findMemberValueElement(paramValueElement, "topics");
    QString topicCountString = getValueFromStruct(paramValueElement, "total_topic_num");
    Q_ASSERT(!topicCountString.isEmpty());
    bool isok = true;
    int topicCount = topicCountString.toInt(&isok);
    Q_ASSERT(isok);
    QDomElement dataElement = topicsValueElement.firstChildElement("array").firstChildElement("data");

    if(topicCount > 0) {
        if(dataElement.nodeName() == "data") {
            getThreads(dataElement, &threads);
        } else {
            qDebug() << Q_FUNC_INFO << docs;
            emit updateFailure(subscription(), "Error while updating group " + groupBeingUpdated->name() + "\nUnexpected TapatTalk reply.\nThis is a bug in Siilihai or TapaTalk server.\nSee console for details.");
            setState(UES_ERROR);
        }
        if(threads.isEmpty()) {
            qDebug() << Q_FUNC_INFO << "Couldn't get any threads in group " << groupBeingUpdated->name() << ". Received XML: \n\n" << docs << "\n\n";
        }
    }

    ForumGroup *groupThatWasBeingUpdated = groupBeingUpdated;
    groupBeingUpdated = 0;
    listThreadsFinished(threads, groupThatWasBeingUpdated);
    qDeleteAll(threads);
}

void TapaTalkEngine::getThreads(QDomElement arrayDataElement, QList<ForumThread *> *threads)
{
    Q_ASSERT(arrayDataElement.nodeName()=="data");
    QDomElement dataValueElement = arrayDataElement.firstChildElement("value");
    if(dataValueElement.nodeName()=="value") {
        while(!dataValueElement.isNull()) {
            QString name = getValueFromStruct(dataValueElement, "topic_title");
            QString id = getValueFromStruct(dataValueElement, "topic_id");
            QString author = getValueFromStruct(dataValueElement, "topic_author_name");
            QString lc = getValueFromStruct(dataValueElement, "last_reply_time");

            if(!id.isNull()) {
                ForumThread *newThread = new ForumThread(this, true);
                newThread->setId(id);
                newThread->setName(name);
                newThread->setLastchange(lc);
                newThread->setGetMessagesCount(subscription()->latestMessages());
                newThread->setOrdernum(threads->size());

                threads->append(newThread);
            }
            dataValueElement = dataValueElement.nextSiblingElement();
        }
    } else {
        emit networkFailure("Unexpected TapaTalk reply while updating threads. This is a bug in Siilihai or TapaTalk server.\nSee console for details.");
        setState(UpdateEngine::UES_ERROR);
    }
}

void TapaTalkEngine::doUpdateThread(ForumThread *thread) {
    if(loginIfNeeded()) return;

    Q_ASSERT(!threadBeingUpdated);
    threadBeingUpdated = thread;

    currentMessagePage = 0; // Start from first page
    qDeleteAll(messages);
    messages.clear();
    updateCurrentThreadPage();
}

void TapaTalkEngine::updateCurrentThreadPage() {
    int firstMessage = currentMessagePage * 50;
    int lastMessage = qMin(firstMessage + 49, threadBeingUpdated->getMessagesCount());

    qDebug() << Q_FUNC_INFO << "Getting messages " << firstMessage <<  " to " << lastMessage << " in " << threadBeingUpdated->toString();

    Q_ASSERT(firstMessage < lastMessage);

    QNetworkRequest req(connectorUrl);

    QList<QPair<QString, QString> > params;
    params.append(QPair<QString, QString>("string", threadBeingUpdated->id()));
    params.append(QPair<QString, QString>("int", QString::number(firstMessage)));
    params.append(QPair<QString, QString>("int", QString::number(lastMessage)));

    QDomDocument doc("");

    createMethodCall(doc, "get_thread", params);

    QByteArray requestData = doc.toByteArray();

    req.setAttribute(QNetworkRequest::User, TTO_UpdateThread);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QString("application/x-www-form-urlencoded"));
    nam.post(req, requestData);
}

void TapaTalkEngine::replyUpdateThread(QNetworkReply *reply) {
    if(!threadBeingUpdated) {
        qDebug() << Q_FUNC_INFO << "We are not updating thread currently - ignoring this";
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        emit networkFailure(QString("Updating thread %1 failed: %2").arg(threadBeingUpdated->toString()).arg(reply->errorString()));

        ForumThread *updatedThread = threadBeingUpdated;
        threadBeingUpdated=0;
        listMessagesFinished(messages, updatedThread, false);
        return;
    }

    QString docs = QString().fromUtf8(reply->readAll());
    QDomDocument doc;
    doc.setContent(docs);
    QDomElement paramValueElement = doc.firstChildElement("methodResponse").firstChildElement("params").firstChildElement("param").firstChildElement("value");
    getMessages(paramValueElement, &messages);

    // Find total_post_num value to figure out if we could have more messages
    QDomElement totalPostNumElement = findMemberValueElement(paramValueElement, "total_post_num");
    QString totalPostNumString = valueElementToString(totalPostNumElement);
    int totalPostNum = totalPostNumString.toInt();
    qDebug() << Q_FUNC_INFO << "Got " << messages.size() << " messages now, total" << totalPostNum << ", limit " << threadBeingUpdated->getMessagesCount();
    if(messages.size() < threadBeingUpdated->getMessagesCount() &&
            messages.size() < totalPostNum) {
        // Need to get next page of messages
        currentMessagePage++;
        updateCurrentThreadPage();
    } else {
        if(messages.isEmpty()) {
            qDebug() << Q_FUNC_INFO << "Couldn't get any messages in thread " << threadBeingUpdated->name() << ". Received XML: \n\n" << docs << "\n\n";
        }

        ForumThread *updatedThread = threadBeingUpdated;
        threadBeingUpdated=0;
        listMessagesFinished(messages, updatedThread, totalPostNum > messages.size());
        qDeleteAll(messages);
        messages.clear();
    }
}

bool TapaTalkEngine::loginIfNeeded() {
    if(loggedIn) return false;
    if(!subscription()->authenticated()) return false;

    QNetworkRequest req(connectorUrl);

    QList<QPair<QString, QString> > params;
    params.append(QPair<QString, QString>("base64", subscription()->username()));
    params.append(QPair<QString, QString>("base64", subscription()->password()));

    QDomDocument doc("");

    createMethodCall(doc, "login", params);

    QByteArray requestData = doc.toByteArray();

    // qDebug() << Q_FUNC_INFO << "TX: " << doc.toString();

    req.setAttribute(QNetworkRequest::User, TTO_Login);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QString("application/x-www-form-urlencoded"));
    nam.post(req, requestData);

    return true;
}

void TapaTalkEngine::replyLogin(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << Q_FUNC_INFO << reply->errorString();
        emit networkFailure(reply->errorString());
        loginFinishedSlot(subscription(), false);
        return;
    }
    QString docs = QString().fromUtf8(reply->readAll());
    // qDebug() << Q_FUNC_INFO << "RX:" << docs;
    QDomDocument doc;
    doc.setContent(docs);

    QDomElement paramValueElement = doc.firstChildElement("methodResponse").firstChildElement("params").firstChildElement("param").firstChildElement("value");
    if(!paramValueElement.isNull()) {
        QString result = getValueFromStruct(paramValueElement, "result");
        if(result=="1") {
            loggedIn = true;
        }
        loginFinishedSlot(subscription(), loggedIn);
    } else {
        qDebug() << Q_FUNC_INFO << "Error in TapaTalk login reply:" << docs;
        emit networkFailure("Received unexpected TapaTalk login reply.\nSee console for details.");
        setState(UpdateEngine::UES_ERROR);
    }
}

void TapaTalkEngine::getMessages(QDomElement dataValueElement, QList<ForumMessage *> *messages) {
    Q_ASSERT(dataValueElement.nodeName()=="value");
    QDomElement postsValueElement = findMemberValueElement(dataValueElement, "posts");
    QDomElement arrayDataValueElement = postsValueElement.firstChildElement("array").firstChildElement("data").firstChildElement("value");
    while(!arrayDataValueElement.isNull()) {
        QString id = getValueFromStruct(arrayDataValueElement, "post_id");
        if(!id.isNull()) {
            ForumMessage *newMessage = new ForumMessage(this, true);
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
    // Contais group id and hierarchy item for getting full name
    QMap<QString, GroupHierarchyItem> groupHierarchy;

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << Q_FUNC_INFO << reply->errorString();
        listGroupsFinished(grps, subscription());
        return;
    }
    QString docs = QString().fromUtf8(reply->readAll());
    QDomDocument doc;
    doc.setContent(docs);
    QDomElement resultElement = doc.firstChildElement("methodResponse").firstChildElement("params").firstChildElement("param").firstChildElement("value");
    qDebug() << Q_FUNC_INFO << resultElement.tagName();
    QString result = getValueFromStruct(resultElement, "result");
    if(result.isEmpty() || result != "0") { // Getting list succeeded
        QDomElement arrayDataElement = doc.firstChildElement("methodResponse").firstChildElement("params").firstChildElement("param").firstChildElement("value").firstChildElement("array").firstChildElement("data");
        if(arrayDataElement.nodeName()=="data") {
            getGroups(arrayDataElement, &grps, groupHierarchy);
            fixGroupNames(&grps, groupHierarchy);
        } else {
            emit updateFailure(subscription(), "Unexpected TapaTalk response while listing groups.\nThis is a bug in Siilihai or server. Check console");
            qDebug() << Q_FUNC_INFO << "Expected data element in response, got " << arrayDataElement.nodeName() << ". Full xml: \n\n" << docs << "\n\n";
        }
    } else { // Getting list failed
        QString resultText = getValueFromStruct(resultElement, "result_text");
        if(resultText.isEmpty()) resultText = "Updating TapaTalk group list failed (no result text)";
        emit updateFailure(subscription(), resultText);
        setState(UpdateEngine::UES_ERROR);
    }
    listGroupsFinished(grps, subscription());
    qDeleteAll(grps);
}

void TapaTalkEngine::getGroups(QDomElement arrayDataElement, QList<ForumGroup *> *grps, QMap<QString, GroupHierarchyItem> &groupHierarchy) {
    Q_ASSERT(arrayDataElement.nodeName()=="data");

    QDomElement arrayValueElement = arrayDataElement.firstChildElement("value");
    while(!arrayValueElement.isNull()) {
        QString groupId = getValueFromStruct(arrayValueElement, "forum_id");
        QString parentId = getValueFromStruct(arrayValueElement, "parent_id");
        QString groupName = getValueFromStruct(arrayValueElement, "forum_name");
        QString subOnly = getValueFromStruct(arrayValueElement, "sub_only");
        // QString newPosts = getValueFromStruct(arrayValueElement, "new_post");

        GroupHierarchyItem ghi;
        ghi.name = groupName;
        ghi.parentId = parentId;

        groupHierarchy.insert(groupId, ghi);

        if(subOnly=="0" || subOnly.isEmpty()) { // Add only leaf groups
            ForumGroup *newGroup = new ForumGroup(this, true);

            newGroup->setName(groupName);
            newGroup->setId(groupId);
            newGroup->setLastchange(QString::number(rand()));
            newGroup->setChangeset(rand()); // TapaTalk doesn't support last change per group

            // newPosts can't work as server doesn't know what we've read
            /*
            if(newPosts=="1") { // Mark as changed
                newGroup->setChangeset(rand());
            }*/

            grps->append(newGroup);
        }
        getChildGroups(arrayValueElement, grps, groupHierarchy);
        arrayValueElement = arrayValueElement.nextSiblingElement();
    }
}

QString TapaTalkEngine::getValueFromStruct(QDomElement arrayValueElement, QString name) {
    Q_ASSERT(arrayValueElement.nodeName()=="value");

    QDomElement valueElement = findMemberValueElement(arrayValueElement, name);
    return valueElementToString(valueElement);
}

QString TapaTalkEngine::valueElementToString(QDomElement valueElement) {
    if(!valueElement.isNull()) {
        QDomElement valueTypeElement = valueElement.firstChildElement();
        if(valueTypeElement.nodeName()=="string") {
            return valueTypeElement.text();
        } else if(valueTypeElement.nodeName()=="base64") {
            QByteArray valueText = QByteArray::fromBase64(valueTypeElement.text().toUtf8());
            return valueText.data();
        } else if(valueTypeElement.nodeName()=="boolean") {
            return valueTypeElement.text();
        } else if(valueTypeElement.nodeName()=="dateTime.iso8601") {
            return valueTypeElement.text();
        } else if(valueTypeElement.nodeName()=="int") {
            return valueTypeElement.text();
        } else {
            qDebug() << Q_FUNC_INFO << "unknown value type " << valueTypeElement.nodeName();
        }
    } else {
        qDebug() << Q_FUNC_INFO << "Null value element given!";
    }
    return QString();
}

void TapaTalkEngine::createMethodCall(QDomDocument & doc, QString method, QList<QPair<QString, QString> > &params) {
    QDomElement methodCallElement = doc.createElement("methodCall");
    doc.appendChild(methodCallElement);

    QDomElement methodTag = doc.createElement("methodName");
    methodCallElement.appendChild(methodTag);

    QDomText t = doc.createTextNode(method);
    methodTag.appendChild(t);

    QDomElement paramsTag = doc.createElement("params");
    methodCallElement.appendChild(paramsTag);
    for(int i=0;i<params.size();i++) {
        QPair<QString, QString> param = params.at(i);
        QDomElement paramTag = doc.createElement("param");
        paramsTag.appendChild(paramTag);

        QDomElement valueTag = doc.createElement("value");
        paramTag.appendChild(valueTag);

        QDomElement valueTypeTag = doc.createElement(param.first);
        valueTag.appendChild(valueTypeTag);

        QByteArray realValue = param.second.toUtf8();

        if(param.first == "base64") {
            realValue = realValue.toBase64();
        }
        QDomText valueText = doc.createTextNode(realValue);
        valueTypeTag.appendChild(valueText);
    }
}

QString TapaTalkEngine::groupHierarchyString(QMap<QString, GroupHierarchyItem> &groupHierarchy, QString id) {
    QString fullHierarchy;
    Q_ASSERT(groupHierarchy.contains(id));
    GroupHierarchyItem ghi = groupHierarchy.value(id);

    // Nice.. both of these seem to mean no parent
    while(ghi.parentId != "-1" && ghi.parentId != "0") {
        if(!groupHierarchy.contains(ghi.parentId)) {
            return "PARENT NOT FOUND! / " + fullHierarchy;
        }
        ghi = groupHierarchy.value(ghi.parentId);
        if(fullHierarchy.isEmpty()) {
            fullHierarchy = ghi.name;
        } else {
            fullHierarchy = ghi.name + " / " + fullHierarchy;
        }
    }
    return fullHierarchy;
}

void TapaTalkEngine::fixGroupNames(QList<ForumGroup *> *grps, QMap<QString, GroupHierarchyItem> &groupHierarchy)
{
    foreach(ForumGroup *grp, *grps) {
        QString hierarchy = groupHierarchyString(groupHierarchy, grp->id());
        grp->setHierarchy(hierarchy);
    }
}

void TapaTalkEngine::getChildGroups(QDomElement arrayValueElement, QList<ForumGroup *> *grps, QMap<QString, GroupHierarchyItem> &groupHierarchy)
{
    Q_ASSERT(arrayValueElement.nodeName()=="value");
    QDomElement valueElement = findMemberValueElement(arrayValueElement, "child");
    if(!valueElement.isNull()) {
        QDomElement dataElement = valueElement.firstChildElement("array").firstChildElement("data");
        if(!dataElement.isNull()) {
            getGroups(dataElement, grps, groupHierarchy);
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
