#ifndef CLIENTLOGIC_H
#define CLIENTLOGIC_H

#include <QObject>
#include <QSettings>

#include <siilihai/parserengine.h>
#include <siilihai/forumdatabasexml.h>
#include <siilihai/syncmaster.h>
#include <siilihai/usersettings.h>

#define BASEURL "http://www.siilihai.com/"
#define MAX_CONCURRENT_UPDATES 2

// State chart:
//                 ,------>--------.
// started -> login -> startsync ->  ready -> endsync -> storedb -> quit
//              | ^       |            |                  ^
//              v |  .--<-+------<-----'                  |
//             offline ------------------------>----------'
//
// @todo switch to Qt's state machine

class ClientLogic : public QObject
{
    Q_OBJECT

public:
    enum siilihai_states {
        SH_STARTED,
        SH_LOGIN,
        SH_STARTSYNCING,
        SH_OFFLINE,
        SH_READY,
        SH_ENDSYNC,
        SH_STOREDB
    } currentState;
    explicit ClientLogic(QObject *parent = 0);

signals:

public slots:
    void launchSiilihai();
    void updateClicked();
    void updateClicked(ForumSubscription* forumid, bool force=false);
    void haltSiilihai();
    void cancelClicked();
    void syncFinished(bool success, QString message);
    void offlineModeSet(bool newOffline);
protected:
    virtual QString getDataFilePath();
    virtual void errorDialog(QString message)=0;
    virtual void settingsChanged(bool byUser);
    virtual void showLoginWizard()=0;
    virtual void changeState(siilihai_states newState);
    virtual void closeUi()=0;
    virtual void loginFinished(bool success, QString motd, bool sync);
    virtual void parserEngineStateChanged(ParserEngine *engine, ParserEngine::ParserEngineState newState, ParserEngine::ParserEngineState oldState);
    virtual void loginWizardFinished();
    virtual void showMainWindow()=0;
    virtual void subscriptionFound(ForumSubscription* sub);
    virtual void unsubscribeForum(ForumSubscription* fs);
    virtual void updateGroupSubscriptions(ForumSubscription *sub);
    QSettings *settings;
    ForumDatabaseXml forumDatabase;
    SiilihaiProtocol protocol;
    QString baseUrl;
    SyncMaster syncmaster;
    ParserManager *parserManager;
private slots:
    virtual void subscribeForum()=0;
    void listSubscriptionsFinished(QList<int> subscriptions);
    void forumAdded(ForumSubscription *fs);
    void subscriptionDeleted(QObject* subobj);
    void forumUpdated(ForumSubscription* forumid);
    void subscribeForumFinished(ForumSubscription *sub, bool success);
    void userSettingsReceived(bool success, UserSettings *newSettings);
    void updateFailure(ForumSubscription* sub, QString msg);
    void moreMessagesRequested(ForumThread* thread);
    void unsubscribeGroup(ForumGroup *group);
    void forumUpdateNeeded(ForumSubscription *sub);
    void unregisterSiilihai();
    void databaseStored();
    void updateThread(ForumThread* thread, bool force=false);
    void forumLoginFinished(ForumSubscription *sub, bool success);
private:
    void tryLogin();

    QHash <ForumSubscription*, ParserEngine*> engines;
    QList<ForumSubscription*> subscriptionsToUpdateLeft;

    UserSettings usettings;
    bool dbStored;
    QNetworkAccessManager nam;
    bool endSyncDone;
    bool firstRun;
};

#endif // CLIENTLOGIC_H
