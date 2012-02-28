#include "credentialsrequest.h"
#include <QDebug>

CredentialsRequest::CredentialsRequest(QObject *parent) :
    QObject(parent), credentialType(SH_CREDENTIAL_NONE), subscription(0)
{
}

CredentialsRequest::CredentialsRequest() : QObject(), credentialType(SH_CREDENTIAL_NONE), subscription(0) {
}

void CredentialsRequest::signalCredentialsEntered(bool store) {
    qDebug() << Q_FUNC_INFO;
    emit credentialsEntered(store);
}
