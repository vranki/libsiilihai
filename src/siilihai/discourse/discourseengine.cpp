#include "discourseengine.h"
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include "../forumdata/forumsubscription.h"
#include "../forumdata/forumgroup.h"
#include "../forumdata/forumthread.h"
#include "../forumdata/forummessage.h"
#include "forumsubscriptiondiscourse.h"

DiscourseEngine::DiscourseEngine(QObject *parent, ForumDatabase *fd) :
    UpdateEngine(parent, fd)
{
    connect(&nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkReply(QNetworkReply*))/*, Qt::UniqueConnection*/);
    connect(&nam, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrors(QNetworkReply*,QList<QSslError>)));
    connect(&nam, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), this, SLOT(authenticationRequired(QNetworkReply*,QAuthenticator*)));
}

void DiscourseEngine::probeUrl(QUrl url)
{
    apiUrl = url;
    QUrl aboutUrl = apiUrl;
    aboutUrl.setPath("/about.json");
    qDebug() << Q_FUNC_INFO << "will now probe " << aboutUrl;

    QNetworkRequest req = QNetworkRequest(aboutUrl);

    req.setAttribute(QNetworkRequest::User, DO_Probe);
    nam.get(req);
}

void DiscourseEngine::replyProbe(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit urlProbeResults(0);
        return;
    }
    QString docs = QString().fromUtf8(reply->readAll());
    QJsonDocument docj = QJsonDocument::fromJson(docs.toUtf8());
    if(!docj.isNull() && docj.isObject() && docj.object().contains("about")) {
        QJsonValue about = docj.object().value("about");
        QString title = about.toObject().value("title").toString();
        ForumSubscription sub(0, true, ForumSubscription::FP_DISCOURSE);
        sub.setAlias(title);
        sub.setForumUrl(apiUrl);
        emit urlProbeResults(&sub);
    } else {
        qDebug() << Q_FUNC_INFO << "No discourse found, got reply: \n" << docs;
        emit urlProbeResults(0);
    }
}

void DiscourseEngine::replyListGroups(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit networkFailure("Got error while updating forum");
        return;
    }
    QString docs = QString().fromUtf8(reply->readAll());
    QJsonDocument docj = QJsonDocument::fromJson(docs.toUtf8());

    QList<ForumGroup*> tempGroups;
    QJsonArray catList = docj.object().value("category_list").toObject().value("categories").toArray();
    for(QJsonValue cat : catList) {
        QJsonObject catObj = cat.toObject();
        // qDebug() << Q_FUNC_INFO << catObj.value("id").toInt() << catObj.value("name").toString() << catObj.value("last_posted_at").toString();
        // qDebug() << Q_FUNC_INFO << catObj.keys();
        ForumGroup *newGroup = new ForumGroup(this, true);

        newGroup->setName(catObj.value("name").toString());
        newGroup->setId(QString::number(catObj.value("id").toInt()));
        newGroup->setLastchange(QString::number(rand()));
        newGroup->setChangeset(rand());
        tempGroups.append(newGroup);
    }

    listGroupsFinished(tempGroups, subscription());
}

void DiscourseEngine::replyListThreads(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit networkFailure("Got error while updating forum");
        return;
    }
    QString docs = QString().fromUtf8(reply->readAll());
    QJsonDocument docj = QJsonDocument::fromJson(docs.toUtf8());

    QList<ForumThread*> tempThreads;
    QJsonArray topicsArray = docj.object().value("topic_list").toObject().value("topics").toArray();
    for(auto topic : topicsArray) {
        QJsonObject topicObject = topic.toObject();
        ForumThread *newThread = new ForumThread(groupBeingUpdated);
        newThread->setId(QString::number(topicObject.value("id").toInt()));
        newThread->setName(topicObject.value("title").toString());
        QDateTime dateTime = QDateTime::fromString(topicObject.value("last_posted_at").toString(), Qt::ISODate);
        newThread->setLastchange(dateTime.toString());
        newThread->setOrdernum(tempThreads.size());
        tempThreads.append(newThread);
    }
    ForumGroup *groupJustUpdated = groupBeingUpdated;
    groupBeingUpdated = 0;
    listThreadsFinished(tempThreads, groupJustUpdated);
}

void DiscourseEngine::replyListMessages(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit networkFailure("Got error while updating forum");
        return;
    }
    QString docs = QString().fromUtf8(reply->readAll());
    QJsonDocument docj = QJsonDocument::fromJson(docs.toUtf8());

    QList<ForumMessage*> tempMessages;
    // qDebug() << Q_FUNC_INFO << docj.object().value("post_stream").toObject().value("posts");
    QJsonArray postsArray = docj.object().value("post_stream").toObject().value("posts").toArray();
    for(auto post : postsArray) {
        QJsonObject postObject = post.toObject();
        qDebug() << Q_FUNC_INFO << postObject.keys();
        ForumMessage *newMessage = new ForumMessage(threadBeingUpdated);
        newMessage->setId(QString::number(postObject.value("id").toInt()));
        QString username = postObject.value("username").toString();
        QString realname = postObject.value("name").toString();
        if(!realname.isEmpty()) username = realname + " (" + username + ")";
        newMessage->setAuthor(username);
        newMessage->setBody(postObject.value("cooked").toString());
        QDateTime dateTime = QDateTime::fromString(postObject.value("updated_at").toString(), Qt::ISODate);
        newMessage->setLastchange(dateTime.toString());
        newMessage->setRead(false);
        newMessage->setOrdernum(tempMessages.size());
        tempMessages.append(newMessage);
    }

    ForumThread *threadJustUpdated = threadBeingUpdated;
    threadBeingUpdated = 0;
    listMessagesFinished(tempMessages, threadJustUpdated, false);
}

ForumSubscriptionDiscourse *DiscourseEngine::subscriptionDiscourse() const
{
    return qobject_cast<ForumSubscriptionDiscourse*>(subscription());
}

void DiscourseEngine::doUpdateForum()
{
    QUrl categoriesUrl = apiUrl;
    categoriesUrl.setPath("/categories.json");

    QNetworkRequest req = QNetworkRequest(categoriesUrl);

    req.setAttribute(QNetworkRequest::User, DO_ListGroups);
    nam.get(req);
}

void DiscourseEngine::doUpdateGroup(ForumGroup *group)
{
    Q_ASSERT(!groupBeingUpdated);

    groupBeingUpdated = group;

    QUrl threadsUrl = apiUrl;
    threadsUrl.setPath(QString("/c/%1.json").arg(group->id()));

    QNetworkRequest req = QNetworkRequest(threadsUrl);

    req.setAttribute(QNetworkRequest::User, DO_UpdateGroup);
    nam.get(req);
}

void DiscourseEngine::doUpdateThread(ForumThread *thread)
{
    qDebug( ) << Q_FUNC_INFO;
    Q_ASSERT(!threadBeingUpdated);

    threadBeingUpdated = thread;

    QUrl messagesUrl = apiUrl;
    messagesUrl.setPath(QString("/t/%1.json").arg(thread->id()));

    QNetworkRequest req = QNetworkRequest(messagesUrl);

    req.setAttribute(QNetworkRequest::User, DO_UpdateThread);
    nam.get(req);
    qDebug() << Q_FUNC_INFO << req.url().toString();
}

void DiscourseEngine::networkReply(QNetworkReply *reply)
{
    qDebug( ) << Q_FUNC_INFO;
    int operationAttribute = reply->request().attribute(QNetworkRequest::User).toInt();
    if(!operationAttribute) {
        qDebug( ) << Q_FUNC_INFO << "Reply " << operationAttribute << " not for me";
        return;
    }
    if(operationAttribute==DO_None) {
        Q_ASSERT(false);
    } else if(operationAttribute==DO_Probe) {
        replyProbe(reply);
    } else if(operationAttribute==DO_ListGroups) {
        replyListGroups(reply);
    } else if(operationAttribute==DO_UpdateGroup) {
        replyListThreads(reply);
    } else if(operationAttribute==DO_UpdateThread) {
        replyListMessages(reply);
    } else {
        Q_ASSERT(false);
    }
    reply->deleteLater();
}

void DiscourseEngine::sslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    Q_UNUSED(errors);
    qDebug() << Q_FUNC_INFO;
    reply->ignoreSslErrors();
}

void DiscourseEngine::authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_UNUSED(authenticator);
    Q_UNUSED(reply);
    qDebug( ) << Q_FUNC_INFO;
}

void DiscourseEngine::encrypted(QNetworkReply *reply)
{
    Q_UNUSED(reply);
    qDebug( ) << Q_FUNC_INFO;
}

void DiscourseEngine::setSubscription(ForumSubscription *fs)
{
    Q_ASSERT(!fs || qobject_cast<ForumSubscriptionDiscourse*>(fs));
    UpdateEngine::setSubscription(fs);
    if(!fs) return;
    if(!fs->forumUrl().isValid()) {
        qDebug() << Q_FUNC_INFO << "Error: subscription has no valid url! This shouldn't happen.";
        subscriptionDiscourse()->setDiscourseEngine(this);
        UpdateEngine::setSubscription(0);
        return;
    }
    subscriptionDiscourse()->setDiscourseEngine(this);
    apiUrl = subscriptionDiscourse()->forumUrl().toString(QUrl::StripTrailingSlash) + "/mobiquo/mobiquo.php";
    setState(UES_IDLE);
}

QString DiscourseEngine::engineTypeName()
{
    return "Discourse";
}
