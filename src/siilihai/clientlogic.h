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

class ParserManager;

class ClientLogic : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString statusMessage READ statusMessage WRITE showStatusMessage NOTIFY statusMessageChanged)
public:

    enum siilihai_states {
        SH_STARTED,
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

signals:
    void statusMessageChanged(QString message);

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
    QSettings *settings;
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
    void forumUpdateNeeded(ForumSubscription *sub); // wat?
    void databaseStored();
    void forumLoginFinished(ForumSubscription *sub, bool success);
    void credentialsEntered(bool store); // from CredentialsRequest
    void accountlessLoginFinished(); // Delayed login success (useless?)
    void clearStatusMessage();
protected:
    CredentialsRequest* currentCredentialsRequest; // If being asked
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
};

#endif // CLIENTLOGIC_H
