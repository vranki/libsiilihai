#ifndef CLIENTLOGIC_H
#define CLIENTLOGIC_H

#include <QObject>
#include <QSettings>
#include <QQueue>
#include <QTimer>
#include <QNetworkSession>

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
#include "subscriptionmanagement.h"

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
    Q_PROPERTY(QObject* forumDatabase READ forumDatabase NOTIFY forumDatabaseChanged)
    Q_PROPERTY(QObject* settings READ settings NOTIFY settingsChangedSignal)
    Q_PROPERTY(SubscriptionManagement* subscriptionManagement READ subscriptionManagement NOTIFY subscriptionManagementChanged)
    // Display credentials dialog if this is shown
    Q_PROPERTY(CredentialsRequest* currentCredentialsRequest READ currentCredentialsRequest NOTIFY currentCredentialsRequestChanged)
    Q_PROPERTY(bool offline READ offline WRITE setOffline NOTIFY offlineChanged)
public:
    enum siilihai_states {
        SH_OFFLINE=0,
        SH_LOGIN,
        SH_STARTSYNCING,
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
    /**
     * Call this if any of the settings that need to be sent to server
     * have changed
     *
     * @brief settingsChanged
     * @param byUser if the user has changed them (not server)
     */
    Q_INVOKABLE virtual void settingsChanged(bool byUser);
    Q_INVOKABLE virtual QString getDataFilePath();

    QObject* forumDatabase();
    QObject* settings();
    SubscriptionManagement *subscriptionManagement();
    CredentialsRequest* currentCredentialsRequest() const;

    bool offline() const;

public slots:
    virtual void launchSiilihai();
    // Call with 0 subscription to update all
    virtual void updateClicked(ForumSubscription* sub=0, bool force=false);
    virtual void haltSiilihai();
    virtual void cancelClicked();
    virtual void syncFinished(bool success, QString message);
    virtual void subscriptionFound(ForumSubscription* sub);
    virtual void errorDialog(QString message)=0;
    virtual void updateGroupSubscriptions(ForumSubscription *sub); // Call after subscriptions have changed (uploads to server)
    virtual void updateAllParsers();
    virtual void unregisterSiilihai();
    virtual void aboutToQuit();
    virtual void setDeveloperMode(bool newDm);
    virtual void subscribeForum()=0; // Display the subscription dialog
    void loginUser(QString user, QString password);
    virtual void loginWizardFinished();
    void setOffline(bool newOffline);

signals:
    void statusMessageChanged(QString message);
    void developerModeChanged(bool arg);
    void forumDatabaseChanged(QObject* forumDatabase);
    void settingsChangedSignal(QObject* settings);
    void subscriptionManagementChanged(SubscriptionManagement *subscriptionManagement);
    void forumSubscribed(ForumSubscription *sub);
    void showLoginWizard();
    void loginFinished(bool success, QString motd, bool sync);
    void currentCredentialsRequestChanged(CredentialsRequest* currentCredentialsRequest);
    void groupListChanged(ForumSubscription* sub); // Show group subscription dialog or whatever
    void offlineChanged(bool offline);

protected:
    virtual void changeState(siilihai_states newState);
    virtual void closeUi()=0;
    virtual void showMainWindow() {}
    virtual bool noAccount(); // True if accountless usage
    SiilihaiSettings *m_settings;
    ForumDatabaseXml m_forumDatabase;
    SiilihaiProtocol m_protocol;
    QString m_baseUrl;
    SyncMaster m_syncmaster;
    ParserManager *m_parserManager;

protected slots:
    virtual void subscriptionDeleted(QObject* subobj);
    virtual void getHttpAuthentication(ForumSubscription *fsub, QAuthenticator *authenticator);
    virtual void getForumAuthentication(ForumSubscription *fsub);
    virtual void showStatusMessage(QString message=QString::null);
    virtual void forumUpdateNeeded(ForumSubscription *sub); // Sends the updated forum info (authentication etc)
    virtual void unsubscribeForum(ForumSubscription* fs);
    void forumAdded(ForumSubscription *fs); // Ownership won't change
    void moreMessagesRequested(ForumThread* thread);
    void unsubscribeGroup(ForumGroup *group);
    void updateForum(ForumSubscription *sub);
    virtual void updateThread(ForumThread* thread, bool force=false);
    void loginFinishedSlot(bool success, QString motd, bool sync);

private slots:
    virtual void updateEngineStateChanged(UpdateEngine *engine, UpdateEngine::UpdateEngineState newState,
                                          UpdateEngine::UpdateEngineState oldState);
    void syncProgress(float progress, QString message);
    void listSubscriptionsFinished(QList<int> subscriptions);
    void forumUpdated(ForumSubscription* forumid);
    void userSettingsReceived(bool success, UserSettings *newSettings);
    void databaseStored();
    void forumLoginFinished(ForumSubscription *sub, bool success);
    void credentialsEntered(bool store); // from CredentialsRequest
    void accountlessLoginFinished(); // Delayed login success (useless?)
    void clearStatusMessage();
    void protocolError(QString message);

protected:
    CredentialsRequest* m_currentCredentialsRequest; // If being asked
    virtual SiilihaiSettings *createSettings(); // Create the settings object to be used. Can be your own subclass.
    QNetworkSession *networkSession;

private:
    void tryLogin();
    void showNextCredentialsDialog();
    int busyForumCount();
    void createEngineForSubscription(ForumSubscription *newFs);

    siilihai_states currentState;
    QHash <ForumSubscription*, UpdateEngine*> engines;
    QList<ForumSubscription*> subscriptionsToUpdateLeft;
    QSet<UpdateEngine*> busyUpdateEngines;
    QSet<ForumSubscription*> subscriptionsNotUpdated; // Subs that never have been updated
    UserSettings usettings;
    bool dbStored;
    QNetworkAccessManager nam;
    bool endSyncDone;
    bool firstRun;
    QQueue<CredentialsRequest*> credentialsRequests;
    QString statusMsg; // One-liner describing current status
    QTimer statusMsgTimer; // Hides the message after a short delay
    SubscriptionManagement *m_subscriptionManagement;
    bool devMode; // Enables some debugging features on UI etc..
};

#endif // CLIENTLOGIC_H
