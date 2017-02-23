#ifndef CREDENTIALSREQUEST_H
#define CREDENTIALSREQUEST_H

#include <QObject>
#include <QAuthenticator>

class ForumSubscription;

class CredentialsRequest : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString forumName READ forumName NOTIFY forumNameChanged)
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)

public:
    explicit CredentialsRequest(ForumSubscription *sub);
    explicit CredentialsRequest(ForumSubscription *sub, QObject *parent);

    enum credential_types {
        SH_CREDENTIAL_NONE=0,
        SH_CREDENTIAL_HTTP,
        SH_CREDENTIAL_FORUM
    } credentialType;

    // Call this when credentials are set.
    Q_INVOKABLE void signalCredentialsEntered(bool store);

    ForumSubscription *subscription();
    QString username() const;
    QString password() const;
    QString forumName() const;

public slots:
    void setUsername(QString username);
    void setPassword(QString password);

signals:
    void credentialsEntered(bool store);
    void usernameChanged(QString username);
    void passwordChanged(QString password);
    void forumNameChanged(QString forumName);
private:
    ForumSubscription *m_subscription;
    QAuthenticator m_authenticator;
};

#endif // CREDENTIALSREQUEST_H
