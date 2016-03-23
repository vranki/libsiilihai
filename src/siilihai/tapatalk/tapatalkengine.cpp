#include "tapatalkengine.h"
#include "forumsubscriptiontapatalk.h"
#include "../forumdata/forumgroup.h"
#include "../forumdata/forumthread.h"
#include "../forumdata/forummessage.h"
#include "../forumdata/updateerror.h"
#include <QDebug>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QByteArray>
#include <QDomDocument>
#include <QString>
#include <QByteArray>
#include <QRegExp>
#include <QDateTime>

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
    apiUrl = subscriptionTapaTalk()->forumUrl().toString(QUrl::StripTrailingSlash) + "/mobiquo/mobiquo.php";
    setState(UES_IDLE);
}

ForumSubscriptionTapaTalk *TapaTalkEngine::subscriptionTapaTalk() const
{
    return qobject_cast<ForumSubscriptionTapaTalk*>(subscription());
}

void TapaTalkEngine::convertBodyToHtml(ForumMessage *msg)
{
    // @todo: this is probably quite stupid way to do this. Use Regexp or something?

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
                urlAddress.replace("[url]", ""); // URL address can't have URL's in them
                urlAddress = "<a href=\"" + urlAddress + "\">";
                newBody = newBody.replace(urlPosition, 5, urlAddress);
            } else {
                // Looks like there is no end tag for this [url]..
                // quit searching.
                urlPosition = -1;
            }
        }
    } while(urlPosition >= 0);

    // Replace [quote name='GlobalGentleman' timestamp='1355152101' post='2541234']
    int quotePosition = -1;
    do {
        quotePosition = newBody.indexOf("[quote");
        if(quotePosition >= 0) {
            int quoteEndPosition = newBody.indexOf("]", quotePosition);
            if(quoteEndPosition >= quotePosition) {
                newBody.replace(quotePosition, quoteEndPosition - quotePosition + 1, "<div class=\"quote\">");
            } else {
                quotePosition = -1;
            }
        }
    } while(quotePosition >=0);

    // newBody.append("----- <pre>" + origBody + "</pre>");
    msg->setBody(newBody);
}

bool TapaTalkEngine::supportsPosting() {
    return true;
}

// Convert to proper ISO 8601 and then to local default format
QString TapaTalkEngine::convertDate(QString &date) {
    QString dateFixed = date;
    dateFixed.insert(6, '-');
    dateFixed.insert(4, '-');
    QDateTime qdd = QDateTime::fromString(dateFixed, Qt::ISODate);
    return qdd.isValid() ? qdd.toString() : date;
}

void TapaTalkEngine::probeUrl(QUrl url)
{
    apiUrl = url.toString(QUrl::StripTrailingSlash) + "/mobiquo/mobiquo.php";
    qDebug() << Q_FUNC_INFO << "will now probe " << apiUrl;
    QNetworkRequest req(apiUrl);
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

    QNetworkRequest req(apiUrl);

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
    QNetworkRequest req(apiUrl);

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
        subscription()->appendError(new UpdateError(QString("Error while updating group %1").arg(groupBeingUpdated->name()),
                                                    "Unexpected TapatTalk reply. This is a bug in Siilihai or TapaTalk server. ",
                                                    reply->errorString()));
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
            subscription()->appendError(new UpdateError(QString("Error while updating group %1").arg(groupBeingUpdated->name()),
                                                        "Unexpected TapatTalk reply. This is a bug in Siilihai or TapaTalk server. ",
                                                        docs));
        }
        if(threads.isEmpty()) {
            subscription()->appendError(new UpdateError(QString("Couldn't get any threads in group %1").arg(groupBeingUpdated->name()),
                                                        "",
                                                        docs));
        }
    }

    ForumGroup *groupThatWasBeingUpdated = groupBeingUpdated;
    groupBeingUpdated = 0;
    listThreadsFinished(threads, groupThatWasBeingUpdated);
    qDeleteAll(threads);
}

void TapaTalkEngine::getThreads(QDomElement arrayDataElement, QList<ForumThread *> *threads)
{
    if(arrayDataElement.nodeName() != "data") {
        qDebug() << Q_FUNC_INFO << "Error: arrayDataElement node name is not 'data'!!";
    }
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
        networkFailure("Unexpected TapaTalk reply while updating threads. This is a bug in Siilihai or TapaTalk server.\nSee console for details.");
    }
}


bool TapaTalkEngine::postMessage(ForumGroup *grp, ForumThread *thr, QString subject, QString body) {
    Q_ASSERT(!groupBeingUpdated);

    QNetworkRequest req(apiUrl);

    QList<QPair<QString, QString> > params;
    params.append(QPair<QString, QString>("string", grp->id()));
    if(thr) {
        params.append(QPair<QString, QString>("string", thr->id()));
    }
    params.append(QPair<QString, QString>("base64", subject));
    params.append(QPair<QString, QString>("base64", body));

    QDomDocument doc("");
    if(thr) {
        createMethodCall(doc, "reply_post", params);
    } else {
        createMethodCall(doc, "new_topic", params);
    }
    QByteArray requestData = doc.toByteArray();
    req.setAttribute(QNetworkRequest::User, TTO_PostMessage);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QString("application/x-www-form-urlencoded"));
    nam.post(req, requestData);
    return true;
}


void TapaTalkEngine::replyPost(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << Q_FUNC_INFO << reply->errorString();
        networkFailure(reply->errorString());
        return;
    }
    QString docs = QString().fromUtf8(reply->readAll());
    qDebug() << Q_FUNC_INFO << "RX:" << docs;
    QDomDocument doc;
    doc.setContent(docs);

    QDomElement paramValueElement = doc.firstChildElement("methodResponse").firstChildElement("params").firstChildElement("param").firstChildElement("value");
    if(!paramValueElement.isNull()) {
        QString result = getValueFromStruct(paramValueElement, "result");
        qDebug() << Q_FUNC_INFO << "Result was " << result;
        if(result!="1") {
            QString resultText = getValueFromStruct(paramValueElement, "result_text");
            networkFailure("Sending message failed:\n" + resultText);
        } else {
            emit messagePosted(subscription());
        }
    } else {
        qDebug() << Q_FUNC_INFO << "Error in TapaTalk reply:" << docs;
        networkFailure("Received unexpected TapaTalk reply.\nSee console for details.");
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

    QNetworkRequest req(apiUrl);

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

void TapaTalkEngine::protocolErrorDetected() {
    networkFailure("Error while updating forum " + subscription()->alias() + "\nUnexpected TapatTalk reply.\nThis is a bug in Siilihai or TapaTalk server.\nSee console for details.");
}

void TapaTalkEngine::replyUpdateThread(QNetworkReply *reply) {
    if(!threadBeingUpdated) {
        qDebug() << Q_FUNC_INFO << "We are not updating thread currently - ignoring this";
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        networkFailure(QString("Updating thread %1 failed: %2").arg(threadBeingUpdated->toString()).arg(reply->errorString()));

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
            messages.size() < totalPostNum && (currentMessagePage+1) * 50 < threadBeingUpdated->getMessagesCount()) {
        /*
        The last in previous if is a bit strange.. For some reason a server returned 102 messages although tried to fetch messages 100-110.
        Consider it defense against buggy server implementation. I hope it breaks nothing.

void TapaTalkEngine::replyUpdateThread(QNetworkReply*) Got  100  messages now, total 102 , limit  110
void TapaTalkEngine::updateCurrentThreadPage() Getting messages  100  to  110  in  "81/335/412244: X-Plane 10 HD Mesh v2 - PREVIEW"
[New Thread 0x7fff89e39700 (LWP 12490)]
void MessageViewWidget::messageSelected(ForumMessage*) Selected message  "1/6/2439/7974: Re: Olisikohan aika tehdä joululahjatilaus ihan itselle/ Read:0" Unreads:  1 1 1
void TapaTalkEngine::replyUpdateThread(QNetworkReply*) Got  100  messages now, total 102 , limit  110
void TapaTalkEngine::updateCurrentThreadPage() Getting messages  150  to  110  in  "81/335/412244: X-Plane 10 HD Mesh v2 - PREVIEW"
ASSERT: "firstMessage < lastMessage" in file siilihai/tapatalk/tapatalkengine.cpp, line 270


        */


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
    if(!subscription()->isAuthenticated()) return false;

    QNetworkRequest req(apiUrl);

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
        loginFinishedSlot(subscription(), false);
        networkFailure(reply->errorString());
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
        networkFailure("Received unexpected TapaTalk login reply.\nSee console for details.");
    }
}

void TapaTalkEngine::getMessages(QDomElement dataValueElement, QList<ForumMessage *> *messages) {
    if(dataValueElement.nodeName()!="value") {
        qDebug() << Q_FUNC_INFO << "Error: dataValueElement node name is not 'value'!!";
        protocolErrorDetected();
        return;
    }
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
    } else if(operationAttribute==TTO_Login) {
        replyLogin(reply);
    } else if(operationAttribute==TTO_PostMessage) {
        replyPost(reply);
    } else {
        Q_ASSERT(false);
    }
    reply->deleteLater();
}


void TapaTalkEngine::replyListGroups(QNetworkReply *reply)
{
    QList<ForumGroup*> grps;
    // Contais group id and hierarchy item for getting full name
    QMap<QString, GroupHierarchyItem> groupHierarchy;

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << Q_FUNC_INFO << reply->errorString();
        listGroupsFinished(grps, subscription());
        subscription()->appendError(new UpdateError("Error updating forum", reply->errorString()));
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
            subscription()->appendError(new UpdateError("Unexpected TapaTalk response while listing groups", "", docs));
        }
    } else { // Getting list failed
        QString resultText = getValueFromStruct(resultElement, "result_text");
        if(resultText.isEmpty()) resultText = "Updating TapaTalk group list failed (no result text)";
        networkFailure(resultText);
    }
    listGroupsFinished(grps, subscription());
    qDeleteAll(grps);
}

void TapaTalkEngine::getGroups(QDomElement arrayDataElement, QList<ForumGroup *> *grps, QMap<QString, GroupHierarchyItem> &groupHierarchy) {
    if(arrayDataElement.nodeName()!="data") {
        qDebug() << Q_FUNC_INFO << "Error: arrayDataElement node name is not 'data'!!";
        protocolErrorDetected();
        return;
    }

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
    if(arrayValueElement.nodeName()!="value") {
        qDebug() << Q_FUNC_INFO << "Error: node name is not 'value'";
        protocolErrorDetected();
        return QString::null;
    }
    QDomElement valueElement = findMemberValueElement(arrayValueElement, name);
    if(valueElement.isNull()) return ""; // Avoid warning about null element
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
    if(arrayValueElement.nodeName()!="value") {
        qDebug() << Q_FUNC_INFO << "Error: node name is not 'value'";
        protocolErrorDetected();
        return;
    }
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
    if(arrayValueElement.nodeName()!="value") {
        qDebug() << Q_FUNC_INFO << "Error: node name is not 'value'";
        protocolErrorDetected();
        return QDomElement();
    }
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
