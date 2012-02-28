#ifndef CREDENTIALSREQUEST_H
#define CREDENTIALSREQUEST_H

#include <QObject>
#include <QAuthenticator>

class ForumSubscription;

class CredentialsRequest : public QObject {
    Q_OBJECT
public:
    explicit CredentialsRequest();
    explicit CredentialsRequest(QObject *parent = 0);
    enum credential_types {
        SH_CREDENTIAL_NONE=0,
        SH_CREDENTIAL_HTTP,
        SH_CREDENTIAL_FORUM
    } credentialType;
    QAuthenticator authenticator;
    ForumSubscription *subscription;
    void signalCredentialsEntered();
signals:
    void credentialsEntered();
};

#endif // CREDENTIALSREQUEST_H
