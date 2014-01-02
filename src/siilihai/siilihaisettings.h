#ifndef SIILIHAISETTINGS_H
#define SIILIHAISETTINGS_H

#include <QSettings>
#define BASEURL "http://www.siilihai.com/"

class SiilihaiSettings : public QSettings
{
    Q_OBJECT
public:
    explicit SiilihaiSettings(const QString &fileName, Format format, QObject *parent = 0);
    bool firstRun();
    void setFirstRun(bool fr);
    QString httpProxy();
    QString baseUrl();
    int databaseSchema();
    void setDatabaseSchema(int sv);
    QString username();
    QString password();
    int threadsPerGroup();
    int messagesPerThread();
    int showMoreCount();
    bool noAccount();
    void setNoAccount(bool na);
    bool syncEnabled();
    QString signature();
    void setSignature(QString sig);
};

#endif // SIILIHAISETTINGS_H
