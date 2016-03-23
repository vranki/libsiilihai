#ifndef DISCOURSEENGINE_H
#define DISCOURSEENGINE_H

#include <QObject>
#include "../updateengine.h"

class DiscourseEngine : public UpdateEngine
{
    Q_OBJECT

public:
    enum DiscourseOperation {
        DO_None=0, DO_ListGroups, DO_UpdateGroup, DO_UpdateThread, DO_Probe, DO_Login, DO_PostMessage
    };

    DiscourseEngine(QObject *parent, ForumDatabase *fd);
    virtual void probeUrl(QUrl url);

protected:
    virtual void doUpdateForum();
    virtual void doUpdateGroup(ForumGroup *group);
    virtual void doUpdateThread(ForumThread *thread);

private:
    void replyProbe(QNetworkReply *reply);

private slots:
    void networkReply(QNetworkReply *reply);
    void sslErrors(QNetworkReply * reply, const QList<QSslError> & errors);
    void authenticationRequired(QNetworkReply * reply, QAuthenticator * authenticator);
    void encrypted(QNetworkReply * reply);
};

#endif // DISCOURSEENGINE_H
