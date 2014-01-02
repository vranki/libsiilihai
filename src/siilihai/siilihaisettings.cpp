#include "siilihaisettings.h"

SiilihaiSettings::SiilihaiSettings(const QString &fileName, Format format, QObject *parent) : QSettings(fileName, format, parent) {
}

bool SiilihaiSettings::firstRun() {
    return value("first_run", true).toBool();
}

void SiilihaiSettings::setFirstRun(bool fr) {
    setValue("first_run", fr);
}

QString SiilihaiSettings::httpProxy()
{
    return value("preferences/http_proxy", "").toString();
}

QString SiilihaiSettings::baseUrl() {
    return value("network/baseurl", BASEURL).toString();
}

int SiilihaiSettings::databaseSchema() {
    return value("forum_database_schema", 0).toInt();
}

void SiilihaiSettings::setDatabaseSchema(int sv) {
    setValue("forum_database_schema", sv);
}

QString SiilihaiSettings::username() {
    return value("account/username", "").toString();
}

QString SiilihaiSettings::password()
{
    return value("account/password", "").toString();
}

int SiilihaiSettings::threadsPerGroup()
{
    return value("preferences/threads_per_group", 20).toInt();
}

int SiilihaiSettings::messagesPerThread()
{
    return value("preferences/messages_per_thread", 20).toInt();
}

int SiilihaiSettings::showMoreCount()
{
    return value("preferences/show_more_count", 30).toInt();
}

bool SiilihaiSettings::noAccount()
{
    return value("account/noaccount", false).toBool();
}

void SiilihaiSettings::setNoAccount(bool na)
{
    setValue("account/noaccount", na);
}

bool SiilihaiSettings::syncEnabled() {
    return value("preferences/sync_enabled", false).toBool();
}

QString SiilihaiSettings::signature()
{
    return value("preferences/signature", " --\nSent using Siilihai web forum reader").toString();
}

void SiilihaiSettings::setSignature(QString sig)
{
    setValue("preferences/signature", sig);
}
