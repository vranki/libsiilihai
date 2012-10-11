#include "tapatalkengine.h"
#include "forumsubscriptiontapatalk.h"
#include "../forumdata/forumgroup.h"
#include "../forumdata/forumthread.h"
#include "../forumdata/forummessage.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QByteArray>
#include <QDomDocument>

TapaTalkEngine::TapaTalkEngine(ForumDatabase *fd, QObject *parent) :
    UpdateEngine(parent, fd)
{
    connect(&nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkReply(QNetworkReply*)), Qt::UniqueConnection);
}

void TapaTalkEngine::setSubscription(ForumSubscription *fs) {
    Q_ASSERT(!fs || qobject_cast<ForumSubscriptionTapaTalk*>(fs));
    UpdateEngine::setSubscription(fs);
    if(!fs) return;
    Q_ASSERT(fs->forumUrl().isValid());
    subscriptionTapaTalk()->setTapaTalkEngine(this);
    setState(PES_IDLE);
}

ForumSubscriptionTapaTalk *TapaTalkEngine::subscriptionTapaTalk() const
{
    return qobject_cast<ForumSubscriptionTapaTalk*>(subscription());
}

void TapaTalkEngine::doUpdateForum() {
    qDebug() << Q_FUNC_INFO << "will now update " << subscriptionTapaTalk()->forumUrl().toString();
    QString connectorUrl = subscriptionTapaTalk()->forumUrl().toString() + "mobiquo/mobiquo.php";
    qDebug() << Q_FUNC_INFO << "URL: " << connectorUrl;

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
    qDebug() << Q_FUNC_INFO << "will now update " << group->toString();
}

void TapaTalkEngine::doUpdateThread(ForumThread *thread)
{
    qDebug() << Q_FUNC_INFO << "will now update " << thread->toString();
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
        }


}

void TapaTalkEngine::replyListGroups(QNetworkReply *reply)
{
    QList<ForumGroup*> grps;
    if (reply->error() == QNetworkReply::NoError) {
        QString docs = QString().fromUtf8(reply->readAll());
        qDebug() << Q_FUNC_INFO << docs;
    }
    listGroupsFinished(grps);
}
