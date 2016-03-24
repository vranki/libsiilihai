#include "discourseengine.h"
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "../forumdata/forumsubscription.h"
#include "../forumdata/forumgroup.h"
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

ForumSubscriptionDiscourse *DiscourseEngine::subscriptionDiscourse() const
{
    return qobject_cast<ForumSubscriptionDiscourse*>(subscription());
}

void DiscourseEngine::doUpdateForum()
{
    qDebug( ) << Q_FUNC_INFO;
    QUrl categoriesUrl = apiUrl;
    categoriesUrl.setPath("/categories.json");

    QNetworkRequest req = QNetworkRequest(categoriesUrl);

    req.setAttribute(QNetworkRequest::User, DO_ListGroups);
    nam.get(req);
    qDebug() << Q_FUNC_INFO << req.url().toString();
}

void DiscourseEngine::doUpdateGroup(ForumGroup *group)
{
    qDebug() << Q_FUNC_INFO;
}

void DiscourseEngine::doUpdateThread(ForumThread *thread)
{
    qDebug() << Q_FUNC_INFO;
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
    }  else if(operationAttribute==DO_ListGroups) {
        replyListGroups(reply);
    } else {
        Q_ASSERT(false);
    }
    reply->deleteLater();
}

void DiscourseEngine::sslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    qDebug() << Q_FUNC_INFO;
    reply->ignoreSslErrors();
}

void DiscourseEngine::authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    qDebug( ) << Q_FUNC_INFO;
}

void DiscourseEngine::encrypted(QNetworkReply *reply)
{
    qDebug( ) << Q_FUNC_INFO;
}

void DiscourseEngine::setSubscription(ForumSubscription *fs)
{
    Q_ASSERT(!fs || qobject_cast<ForumSubscriptionDiscourse*>(fs));
    UpdateEngine::setSubscription(fs);
    if(!fs) return;
    Q_ASSERT(fs->forumUrl().isValid());
    subscriptionDiscourse()->setDiscourseEngine(this);
    apiUrl = subscriptionDiscourse()->forumUrl().toString(QUrl::StripTrailingSlash) + "/mobiquo/mobiquo.php";
    setState(UES_IDLE);
}
