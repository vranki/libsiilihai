#ifndef SIILIHAISETTINGS_H
#define SIILIHAISETTINGS_H

#include <QSettings>
#define BASEURL "http://www.siilihai.com/"

class SiilihaiSettings : public QSettings
{
    Q_OBJECT
    Q_PROPERTY(QString signature READ signature WRITE setSignature NOTIFY changed)
    Q_PROPERTY(int threadsPerGroup READ threadsPerGroup WRITE setThreadsPerGroup NOTIFY changed)
    Q_PROPERTY(int messagesPerThread READ messagesPerThread WRITE setMessagesPerThread NOTIFY changed)
    Q_PROPERTY(int showMoreCount READ showMoreCount WRITE setShowMoreCount NOTIFY changed)
    Q_PROPERTY(bool syncEnabled READ syncEnabled WRITE setSyncEnabled NOTIFY changed)
    Q_PROPERTY(bool noAccount READ noAccount WRITE setNoAccount NOTIFY changed)
    Q_PROPERTY(QString httpProxy READ httpProxy WRITE setHttpProxy NOTIFY changed)
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY changed)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY changed)

public:
    explicit SiilihaiSettings(const QString &fileName, Format format, QObject *parent = 0);
    bool firstRun();
    void setFirstRun(bool fr);
    void setHttpProxy(QString url);
    QString httpProxy();
    QString baseUrl();
    int databaseSchema();
    void setDatabaseSchema(int sv);
    void setUsername(QString un);
    QString username();
    void setPassword(QString pass);
    QString password();

    void setThreadsPerGroup(int tpg);
    int threadsPerGroup();
    void setMessagesPerThread(int mpt);
    int messagesPerThread();

    void setShowMoreCount(int smc);
    int showMoreCount();
    bool noAccount();
    void setNoAccount(bool na);
    void setSyncEnabled(bool se);
    bool syncEnabled();
    QString signature();
    void setSignature(QString sig);
    Q_INVOKABLE int maxThreadsPerGroup() const;
    Q_INVOKABLE int maxMessagesPerThread() const;

signals:
    void changed();
};

#endif // SIILIHAISETTINGS_H
