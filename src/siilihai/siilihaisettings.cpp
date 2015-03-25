#include "siilihaisettings.h"

SiilihaiSettings::SiilihaiSettings(const QString &fileName, Format format, QObject *parent) : QSettings(fileName, format, parent) {
}

bool SiilihaiSettings::firstRun() {
    return value("first_run", true).toBool();
}

void SiilihaiSettings::setFirstRun(bool fr) {
    setValue("first_run", fr);
}

void SiilihaiSettings::setHttpProxy(QString url)
{
    setValue("preferences/http_proxy", url);
    emit changed();
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

void SiilihaiSettings::setUsername(QString un)
{
    setValue("account/username", un);
    emit changed();
}

QString SiilihaiSettings::username() {
    return value("account/username", "").toString();
}

void SiilihaiSettings::setPassword(QString pass)
{
    setValue("account/password", pass);
    emit changed();
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

void SiilihaiSettings::setShowMoreCount(int smc)
{
    setValue("preferences/show_more_count", smc);
    emit changed();
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
    emit changed();
}

void SiilihaiSettings::setSyncEnabled(bool se)
{
    setValue("preferences/sync_enabled", se);
    emit changed();
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
    emit changed();
}

int SiilihaiSettings::maxThreadsPerGroup() const {
    return 100;
}

int SiilihaiSettings::maxMessagesPerThread() const {
    return 100;
}

bool SiilihaiSettings::updateFailed(int fid)
{
    QString key = QString("authentication/%1/failed").arg(fid);
    QString valueS = value(key).toString();
    return valueS == "true";
}

void SiilihaiSettings::setUpdateFailed(int fid, bool failed)
{
    QString key = QString("authentication/%1/failed").arg(fid);
    setValue(key, failed ? "true" : "false");
}

void SiilihaiSettings::setThreadsPerGroup(int tpg)
{
    setValue("preferences/threads_per_group", tpg);
    emit changed();
}

void SiilihaiSettings::setMessagesPerThread(int mpt)
{
    setValue("preferences/messages_per_thread", mpt);
    emit changed();
}
