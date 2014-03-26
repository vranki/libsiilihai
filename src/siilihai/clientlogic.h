#ifndef CLIENTLOGIC_H
#define CLIENTLOGIC_H

#include <QObject>
#include <QSettings>
#include <QQueue>
#include <QTimer>

#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

#include "updateengine.h"
#include "forumdatabase/forumdatabasexml.h"
#include "syncmaster.h"
#include "usersettings.h"
#include "credentialsrequest.h"


#define MAX_CONCURRENT_UPDATES 2

// State chart:
//                 ,------>--------.
// started -> login -> startsync ->  ready -> endsync -> storedb -> quit
//              | ^       |            |                  ^
//              v |  .--<-+------<-----'                  |
//             offline ------------------------>----------'
//
// @todo switch to Qt's state machine

class ParserManager;
class SiilihaiSettings;

class ClientLogic : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString statusMessage READ statusMessage WRITE showStatusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool developerMode READ developerMode WRITE setDeveloperMode NOTIFY developerModeChanged)
public:

    enum siilihai_states {
        SH_STARTED=0,
        SH_LOGIN,
        SH_STARTSYNCING,
        SH_OFFLINE,
        SH_READY,
        SH_ENDSYNC,
        SH_STOREDB
    };

    explicit ClientLogic(QObject *parent = 0);
    QString statusMessage() const;
    siilihai_states state() const;
    Q_INVOKABLE bool developerMode() const;

    // Helpers for message posting
    Q_INVOKABLE static QString addReToSubject(QString subject); // Adds Re: to subject if needed
    Q_INVOKABLE static QString addQuotesToBody(QString body); // Strips html and adds [quote] [/quote] around body (will it work everywhere?)

public slots:
    virtual void launchSiilihai(bool offline=false);
    // Call with 0 subscription to update all
    virtual void updateClicked(ForumSubscription* sub=0, bool force=false);
    virtual void haltSiilihai();
    virtual void cancelClicked();
    virtual void syncFinished(bool success, QString message);
    virtual void offlineModeSet(bool newOffline);
    virtual void subscriptionFound(ForumSubscription* sub);
    virtual void errorDialog(QString message)=0;
    virtual void loginFinished(bool success, QString motd, bool sync);
    virtual void unsubscribeForum(ForumSubscription* fs);
    virtual void updateGroupSubscriptions(ForumSubscription *sub);
    virtual void updateAllParsers();
    virtual void unregisterSiilihai();
    virtual void aboutToQuit();
    virtual void setDeveloperMode(bool newDm);

signals:
    void statusMessageChanged(QString message);

    void developerModeChanged(bool arg);

protected:
    virtual QString getDataFilePath();
    virtual void settingsChanged(bool byUser);
    virtual void showLoginWizard()=0;
    virtual void showCredentialsDialog()=0; // currentCredentialsRequest contains the request here
    virtual void changeState(siilihai_states newState);
    virtual void closeUi()=0;
    virtual void loginWizardFinished();
    virtual void showMainWindow()=0;
    virtual bool noAccount(); // True if accountless usage
    SiilihaiSettings *settings;
    ForumDatabaseXml forumDatabase;
    SiilihaiProtocol protocol;
    QString baseUrl;
    SyncMaster syncmaster;
    ParserManager *parserManager;

protected slots:
    virtual void subscriptionDeleted(QObject* subobj);
    virtual void getHttpAuthentication(ForumSubscription *fsub, QAuthenticator *authenticator);
    virtual void getForumAuthentication(ForumSubscription *fsub);
    virtual void showStatusMessage(QString message=QString::null);
    virtual void groupListChanged(ForumSubscription* sub) {}; // Show group subscription dialog or whatever
    virtual void forumUpdateNeeded(ForumSubscription *sub); // Sends the updated forum info (authentication etc)
    void forumAdded(ForumSubscription *fs); // Ownership won't change
    void moreMessagesRequested(ForumThread* thread);
    void unsubscribeGroup(ForumGroup *group);
    void updateForum(ForumSubscription *sub);
    virtual void updateThread(ForumThread* thread, bool force=false);

private slots:
    virtual void subscribeForum()=0;
    virtual void parserEngineStateChanged(UpdateEngine::UpdateEngineState newState,
                                          UpdateEngine::UpdateEngineState oldState);
    void syncProgress(float progress, QString message);
    void listSubscriptionsFinished(QList<int> subscriptions);
    void forumUpdated(ForumSubscription* forumid);
    void subscribeForumFinished(ForumSubscription *sub, bool success);
    void userSettingsReceived(bool success, UserSettings *newSettings);
    void updateFailure(ForumSubscription* sub, QString msg);
    void databaseStored();
    void forumLoginFinished(ForumSubscription *sub, bool success);
    void credentialsEntered(bool store); // from CredentialsRequest
    void accountlessLoginFinished(); // Delayed login success (useless?)
    void clearStatusMessage();
protected:
    CredentialsRequest* currentCredentialsRequest; // If being asked
    virtual SiilihaiSettings *createSettings(); // Create the settings object to be used. Can be your own subclass.
private:
    void tryLogin();
    void showNextCredentialsDialog();
    int busyForumCount();
    void createEngineForSubscription(ForumSubscription *newFs);

    siilihai_states currentState;
    QHash <ForumSubscription*, UpdateEngine*> engines;
    QList<ForumSubscription*> subscriptionsToUpdateLeft;
    QSet<UpdateEngine*> busyParserEngines;
    QSet<ForumSubscription*> subscriptionsNotUpdated; // Subs that never have been updated
    UserSettings usettings;
    bool dbStored;
    QNetworkAccessManager nam;
    bool endSyncDone;
    bool firstRun;
    QQueue<CredentialsRequest*> credentialsRequests;
    QString statusMsg; // One-liner describing current status
    QTimer statusMsgTimer; // Hides the message after a short delay

    bool devMode; // Enables some debugging features on UI etc..
    bool m_developerMode;
};

#endif // CLIENTLOGIC_H
