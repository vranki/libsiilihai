#include "credentialsrequest.h"
#include <QDebug>
#include "forumdata/forumsubscription.h"

CredentialsRequest::CredentialsRequest(ForumSubscription *sub, QObject *parent) :
    QObject(parent),
    credentialType(SH_CREDENTIAL_NONE),
    m_subscription(sub)
{
}

ForumSubscription *CredentialsRequest::subscription()
{
    return m_subscription;
}


void CredentialsRequest::signalCredentialsEntered(bool store) {
    qDebug() << Q_FUNC_INFO << "store:" << store;
    emit credentialsEntered(store);
}

QString CredentialsRequest::username() const
{
    return m_authenticator.user();
}

QString CredentialsRequest::password() const
{
    return m_authenticator.password();
}

QString CredentialsRequest::forumName() const
{
    return m_subscription->alias();
}

void CredentialsRequest::setUsername(QString username)
{
    if (this->username() == username)
        return;
    m_authenticator.setUser(username);
    emit usernameChanged(username);
}

void CredentialsRequest::setPassword(QString password)
{
    if (this->password() == password)
        return;
    m_authenticator.setPassword(password);
    emit passwordChanged(password);
}
