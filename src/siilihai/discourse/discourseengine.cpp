#include "discourseengine.h"
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

DiscourseEngine::DiscourseEngine(QObject *parent, ForumDatabase *fd) :
    UpdateEngine(parent, fd)
{
    connect(&nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkReply(QNetworkReply*))/*, Qt::UniqueConnection*/);
    connect(&nam, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrors(QNetworkReply*,QList<QSslError>)));
    connect(&nam, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), this, SLOT(authenticationRequired(QNetworkReply*,QAuthenticator*)));
}

void DiscourseEngine::probeUrl(QUrl url)
{
    apiUrl = url.toString(QUrl::StripTrailingSlash) + "categories.json";
    qDebug() << Q_FUNC_INFO << "will now probe " << apiUrl;

    QNetworkRequest req = QNetworkRequest(QUrl(apiUrl));

    req.setAttribute(QNetworkRequest::User, DO_Probe);
    nam.get(req);
    qDebug() << Q_FUNC_INFO << req.url().toString();
}

void DiscourseEngine::doUpdateForum()
{

}

void DiscourseEngine::doUpdateGroup(ForumGroup *group)
{

}

void DiscourseEngine::doUpdateThread(ForumThread *thread)
{

}

void DiscourseEngine::replyProbe(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit urlProbeResults(0);
        return;
    }
    QString docs = QString().fromUtf8(reply->readAll());
    qDebug( ) << Q_FUNC_INFO << docs;
    QJsonDocument docj = QJsonDocument::fromJson(docs.toUtf8());
    qDebug() << Q_FUNC_INFO << docj.isNull() << docj.isObject() << docj.object().keys();
    emit urlProbeResults(0);
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
