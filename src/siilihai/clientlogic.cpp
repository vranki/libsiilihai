#include "clientlogic.h"
#include <time.h>
#include <QDir>
#include <QNetworkProxy>
#include <QTimer>
#include <QNetworkConfigurationManager>
#include "parser/parsermanager.h"
#include "forumdata/forumsubscription.h"
#include "forumdata/forumgroup.h"
#include "forumdata/forumthread.h"
#include "credentialsrequest.h"
#include "parser/forumparser.h"
#include "parser/forumsubscriptionparsed.h"
#include "parser/parserengine.h"
#include "tapatalk/forumsubscriptiontapatalk.h"
#include "tapatalk/tapatalkengine.h"
#include "siilihaisettings.h"
#include "messageformatting.h"
#include "subscriptionmanagement.h"

ClientLogic::ClientLogic(QObject *parent) : QObject(parent), m_settings(0),
    m_forumDatabase(this),  m_syncmaster(this, m_forumDatabase, m_protocol),
    m_parserManager(0), currentCredentialsRequest(0), currentState(SH_OFFLINE),
    m_subscriptionManagement(0) {
    endSyncDone = false;
    firstRun = true;
    dbStored = false;
    devMode = 0;
    srand ( time(NULL) );
    QNetworkConfigurationManager ncm;
    QNetworkConfiguration config = ncm.defaultConfiguration();
    networkSession = new QNetworkSession(config, this);
    statusMsgTimer.setSingleShot(true);

    connect(&statusMsgTimer, SIGNAL(timeout()), this, SLOT(clearStatusMessage()));

    // Make sure Siilihai::subscriptionFound is called first to get ParserEngine
    connect(&m_forumDatabase, SIGNAL(subscriptionFound(ForumSubscription*)), this, SLOT(subscriptionFound(ForumSubscription*)));
    connect(&m_forumDatabase, SIGNAL(databaseStored()), this, SLOT(databaseStored()), Qt::QueuedConnection);
    connect(&m_protocol, SIGNAL(userSettingsReceived(bool,UserSettings*)), this, SLOT(userSettingsReceived(bool,UserSettings*)));
    connect(&m_syncmaster, SIGNAL(syncProgress(float, QString)), this, SLOT(syncProgress(float, QString)));
}

QString ClientLogic::statusMessage() const
{
    return statusMsg;
}

ClientLogic::siilihai_states ClientLogic::state() const {
    return currentState;
}

bool ClientLogic::developerMode() const {
    return devMode;
}

QString ClientLogic::addReToSubject(QString subject) {
    if(subject.startsWith("Re:"))
        return subject;
    else
        return "Re: " + subject;
}

QString ClientLogic::addQuotesToBody(QString body) {
    body = MessageFormatting::stripHtml(body);
    body = "[quote]\n" + body + "\n[/quote]\n\n";
    return body;
}

void ClientLogic::launchSiilihai(bool offline) {
    changeState(SH_STARTED);
    m_settings = createSettings();
    emit settingsChangedSignal(m_settings);

    // Create SM only after creating settings, as it needs it.
    m_subscriptionManagement = new SubscriptionManagement(0, &m_protocol, m_settings);
    connect(m_subscriptionManagement, SIGNAL(forumUnsubscribed(ForumSubscription*)), this, SLOT(unsubscribeForum(ForumSubscription*)));
    connect(m_subscriptionManagement, SIGNAL(showError(QString)), this, SLOT(errorDialog(QString)));
    connect(m_subscriptionManagement, SIGNAL(forumAdded(ForumSubscription*)), this, SLOT(forumAdded(ForumSubscription*)));
    emit subscriptionManagementChanged(m_subscriptionManagement);

    firstRun = m_settings->firstRun();
    m_settings->setFirstRun(false);

    QString proxy = m_settings->httpProxy();
    if (!proxy.isEmpty()) {
        QUrl proxyUrl = QUrl(proxy);
        if (proxyUrl.isValid()) {
            QNetworkProxy nproxy(QNetworkProxy::HttpProxy, proxyUrl.host(), proxyUrl.port(0));
            QNetworkProxy::setApplicationProxy(nproxy);
        } else {
            errorDialog("Warning: http proxy is not valid URL");
        }
    }

    QDir dataDir(getDataFilePath());
    if(!dataDir.exists()) dataDir.mkpath(getDataFilePath());

    m_parserManager = new ParserManager(this, &m_protocol);
    m_parserManager->openDatabase(getDataFilePath() + "/siilihai_parsers.xml");

    settingsChanged(false);
    connect(&m_syncmaster, SIGNAL(syncFinished(bool, QString)), this, SLOT(syncFinished(bool, QString)));

    m_protocol.setBaseURL(m_settings->baseUrl());

    QString databaseFileName = getDataFilePath() + "/siilihai_forums.xml";

    qDebug() << Q_FUNC_INFO << "Using data files under " << getDataFilePath();

    if(firstRun) {
        m_forumDatabase.openDatabase(databaseFileName); // Fails
    } else {
        int currentSchemaVersion = m_settings->databaseSchema();
        if(m_forumDatabase.schemaVersion() != currentSchemaVersion) {
            errorDialog("The database schema has been changed. Your forum database will be reset. Sorry. ");
            m_forumDatabase.openDatabase(databaseFileName, false);
        } else {
            if(!m_forumDatabase.openDatabase(databaseFileName)) {
                errorDialog("Could not open Siilihai's forum database file.\n"
                            "See console for details.");
            }
        }
    }
    m_settings->setDatabaseSchema(m_forumDatabase.schemaVersion());
    m_settings->sync();

    if (m_settings->username().isEmpty() && !noAccount()) {
        showLoginWizard();
    } else {
        showMainWindow();
        if(!offline) {
            tryLogin();
        } else {
            offlineModeSet(true);
        }
    }
}

void ClientLogic::haltSiilihai() {
    cancelClicked();

    foreach(UpdateEngine *engine, engines.values()) {
        engine->subscription()->engineDestroyed();
        engine->setSubscription(0);
        engine->deleteLater();
    }

    engines.clear();
    if(usettings.syncEnabled() && !endSyncDone && currentState == SH_READY) {
        qDebug() << "Sync enabled - running end sync";
        changeState(SH_ENDSYNC);
    } else {
        if(currentState != SH_STOREDB && !dbStored) {
            changeState(SH_STOREDB);
        } else {
            qDebug() << Q_FUNC_INFO << "All done - quitting";
            m_settings->sync();
            closeUi();
        }
    }
}

QString ClientLogic::getDataFilePath() {
#if QT_VERSION < 0x050000
    return QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#else
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#endif
}

QObject *ClientLogic::forumDatabase()
{
    return qobject_cast<QObject*> (&m_forumDatabase);
}

QObject *ClientLogic::settings()
{
    return qobject_cast<QObject*> (m_settings);
}

SubscriptionManagement *ClientLogic::subscriptionManagement()
{
    return m_subscriptionManagement;
}

void ClientLogic::settingsChanged(bool byUser) {
    usettings.setSyncEnabled(m_settings->syncEnabled());
    if(byUser && !noAccount()) {
        m_protocol.setUserSettings(&usettings);
    }
    m_settings->sync();
    if(usettings.syncEnabled()) { // Force upsync of everything
        for(ForumSubscription *sub : m_forumDatabase) {
            foreach(ForumGroup *group, sub->values()) {
                group->setHasChanged(true);
            }
        }
    }
}

void ClientLogic::tryLogin() {
    Q_ASSERT(currentState==SH_STARTED || currentState==SH_OFFLINE);
    changeState(SH_LOGIN);
    if(noAccount()) {
        QTimer::singleShot(50, this, SLOT(accountlessLoginFinished()));
        return;
    }
    if(networkSession->isOpen()) {
        connect(&m_protocol, SIGNAL(loginFinished(bool, QString,bool)), this, SLOT(loginFinished(bool, QString,bool)));
        m_protocol.login(m_settings->username(), m_settings->password());
    } else {
        loginFinished(false, "Offline mode", usettings.syncEnabled());
        changeState(SH_OFFLINE);
    }
}

void ClientLogic::accountlessLoginFinished() {
    loginFinished(true, "", false);
}

void ClientLogic::clearStatusMessage() {
    showStatusMessage();
}

SiilihaiSettings *ClientLogic::createSettings() {
    return new SiilihaiSettings(getDataFilePath() + "/siilihai_settings.ini", QSettings::IniFormat, this);
}

void ClientLogic::changeState(siilihai_states newState) {
    siilihai_states previousState = currentState;
    currentState = newState;

    if(newState==SH_OFFLINE) {
        qDebug() << Q_FUNC_INFO << "Offline";
        networkSession->close();
        Q_ASSERT(previousState==SH_LOGIN || previousState==SH_STARTSYNCING || previousState==SH_READY || previousState==SH_STARTED);
        if(previousState==SH_STARTSYNCING) m_syncmaster.cancel();
    } else if(newState==SH_LOGIN) {
        qDebug() << Q_FUNC_INFO << "Login";
        networkSession->open();
        showStatusMessage("Logging in..");
    } else if(newState==SH_STARTSYNCING) {
        qDebug() << Q_FUNC_INFO << "Startsync";
        if(usettings.syncEnabled()) {
            for(ForumSubscription *sub : m_forumDatabase) {
                sub->setScheduledForUpdate(true);
            }
            connect(&m_syncmaster, SIGNAL(syncFinishedFor(ForumSubscription*)), this, SLOT(updateForum(ForumSubscription*)));
            m_syncmaster.startSync();
        }
    } else if(newState==SH_ENDSYNC) {
        qDebug() << Q_FUNC_INFO << "Endsync";
        showStatusMessage("Synchronizing with server..");
        disconnect(&m_syncmaster, SIGNAL(syncFinishedFor(ForumSubscription*)), this, SLOT(updateForum(ForumSubscription*)));
        m_syncmaster.endSync();
    } else if(newState==SH_STOREDB) {
        qDebug() << Q_FUNC_INFO << "Storedb";
        if(!m_forumDatabase.storeDatabase()) {
            errorDialog("Failed to save forum database file");
        }
        dbStored = true;
    } else if(newState==SH_READY) {
        qDebug() << Q_FUNC_INFO << "Ready";
        if(!usettings.syncEnabled())
            updateClicked();

        if (m_forumDatabase.isEmpty()) { // Display subscribe dialog if none subscribed
            subscribeForum();
        }
    }
}

void ClientLogic::updateForum(ForumSubscription *sub) {
    if(!engines.contains(sub)) return; // Can happen if quitting

    subscriptionsNotUpdated.remove(sub);

    if(sub->beingUpdated())
        return;

    if(sub->scheduledForSync() || sub->beingSynced()) {
        qDebug() << Q_FUNC_INFO << sub->toString() << "syncing, not updating";
        return;
    }
    // Don't update if already updating
    if(!sub->scheduledForUpdate())
        sub->setScheduledForUpdate(true);

    int busyForums = busyForumCount();
    if(busyForums < MAX_CONCURRENT_UPDATES) {
        sub->setScheduledForUpdate(false);
        subscriptionsToUpdateLeft.removeAll(sub);
        Q_ASSERT(!sub->beingSynced());
        engines.value(sub)->updateForum();
    } else {
        Q_ASSERT(!sub->beingSynced());
        if(!subscriptionsToUpdateLeft.contains(sub)) subscriptionsToUpdateLeft.append(sub);
    }
}

void ClientLogic::updateClicked(ForumSubscription* sub , bool force) {
    if(currentState != SH_READY) return;

    if(sub) {
        Q_ASSERT(engines.contains(sub));
        if(sub->scheduledForSync() || sub->beingSynced()) {
            qDebug() << Q_FUNC_INFO << sub->toString() << "syncing, not updating";
            return;
        }
        UpdateEngine *engine = engines.value(sub);
        if(engine && (engine->state()==UpdateEngine::UES_IDLE || engine->state()==UpdateEngine::UES_ERROR)
                && currentState != SH_OFFLINE && currentState != SH_STARTED) {
            sub->setScheduledForUpdate(false);
            subscriptionsToUpdateLeft.removeAll(sub);
            subscriptionsNotUpdated.remove(sub);
            Q_ASSERT(!engine->subscription()->beingSynced());
            engine->updateForum(force);
        }
    } else { // Update all
        foreach(UpdateEngine* engine, engines.values()) {
            updateForum(engine->subscription());
        }
    }
}

void ClientLogic::cancelClicked() {
    for(ForumSubscription *sub : m_forumDatabase) {
        sub->setScheduledForUpdate(false);
    }
    foreach(UpdateEngine* engine, engines.values()) {
        engine->cancelOperation();
    }
}


int ClientLogic::busyForumCount() {
    int busyForums = 0;
    for(ForumSubscription *sub : m_forumDatabase) {
        if(sub->updateEngine() && sub->updateEngine()->state()==UpdateEngine::UES_UPDATING) {
            busyForums++;
        }
    }
    return busyForums;
}

void ClientLogic::syncFinished(bool success, QString message){
    qDebug() << Q_FUNC_INFO << success;
    if (!success) {
        errorDialog(QString("Syncing status to server failed.\n\n%1").arg(message));
    }
    if(currentState == SH_STARTSYNCING) {
        foreach(ForumSubscription *sub, subscriptionsNotUpdated) {
            updateForum(sub);
        }
        changeState(SH_READY);
    } else if(currentState == SH_ENDSYNC) {
        endSyncDone = true;
        haltSiilihai();
    }
}

void ClientLogic::offlineModeSet(bool newOffline) {
    qDebug() << Q_FUNC_INFO << newOffline;
    if(newOffline && currentState == SH_READY) {
        changeState(SH_OFFLINE);
    } else if(!newOffline && currentState == SH_OFFLINE) {
        tryLogin();
    }
}

void ClientLogic::listSubscriptionsFinished(QList<int> serversSubscriptions) {
    disconnect(&m_protocol, SIGNAL(listSubscriptionsFinished(QList<int>)), this, SLOT(listSubscriptionsFinished(QList<int>)));

    QList<ForumSubscription*> unsubscribedForums;
    for(ForumSubscription* sub : m_forumDatabase) {
        bool found = false;
        if(sub->provider() == ForumSubscription::FP_PARSER) {
            foreach(int serverSubscriptionId, serversSubscriptions) {
                if(serverSubscriptionId == qobject_cast<ForumSubscriptionParsed*>(sub)->parserId())
                    found = true;
            }
            if (!found) {
                unsubscribedForums.append(sub);
            }
        }
    }
    foreach (ForumSubscription *sub, unsubscribedForums) {
        if(sub->provider() == ForumSubscription::FP_PARSER) {
            m_parserManager->deleteParser(qobject_cast<ForumSubscriptionParsed*>(sub)->parserId());
        }
        if(sub->provider() == ForumSubscription::FP_PARSER) // @todo not tapatalk yet!
            m_forumDatabase.deleteSubscription(sub);
    }
}


void ClientLogic::loginFinished(bool success, QString motd, bool sync) {
    disconnect(&m_protocol, SIGNAL(loginFinished(bool, QString,bool)), this, SLOT(loginFinished(bool, QString,bool)));

    if (success) {
        connect(&m_protocol, SIGNAL(sendParserReportFinished(bool)), this, SLOT(sendParserReportFinished(bool)));
        usettings.setSyncEnabled(sync);
        m_settings->setSyncEnabled(usettings.syncEnabled());
        m_settings->sync();
        if(usettings.syncEnabled()) {
            changeState(SH_STARTSYNCING);
        } else {
            changeState(SH_READY);
        }
    } else {
        errorDialog("Login failed. \n" + motd);
        // @todo this happens also on first run if logging in with wrong u/p.
        // using login as network test sucks anyway.

        // changeState(SH_OFFLINE);
    }
}

// Note: fs *MUST* be a real ForumSubscription derived class, NOT just ForumSubscription with provider set!
void ClientLogic::forumAdded(ForumSubscription *fs) {
    Q_ASSERT(fs->id());

    if(m_forumDatabase.contains(fs->id())) {
        errorDialog("You have already subscribed to " + fs->alias());
    } else {
        ForumSubscription *newFs = ForumSubscription::newForProvider(fs->provider(), &m_forumDatabase, false);

        Q_ASSERT(newFs);
        newFs->copyFrom(fs);
        if(!m_forumDatabase.addSubscription(newFs)) { // Emits subscriptionFound
            errorDialog("Error: Unable to subscribe to forum. Check the log.");
            return;
        }
        fs = 0;
        createEngineForSubscription(newFs);
        Q_ASSERT(engines.value(newFs));
        engines.value(newFs)->updateGroupList();
        if(!noAccount())
            m_protocol.subscribeForum(newFs);
    }
}

void ClientLogic::createEngineForSubscription(ForumSubscription *newFs) {
    Q_ASSERT(!newFs->isTemp());
    if(engines.contains(newFs)) return;
    UpdateEngine *ue = UpdateEngine::newForSubscription(newFs, &m_forumDatabase, m_parserManager);
    Q_ASSERT(ue);
    Q_ASSERT(!engines.contains(newFs));
    engines.insert(newFs, ue);
    ue->setSubscription(newFs);
    connect(ue, SIGNAL(groupListChanged(ForumSubscription*)), this, SLOT(groupListChanged(ForumSubscription*)));
    connect(ue, SIGNAL(forumUpdated(ForumSubscription*)), this, SLOT(forumUpdated(ForumSubscription*)));
    connect(ue, SIGNAL(getHttpAuthentication(ForumSubscription*, QAuthenticator*)), this, SLOT(getHttpAuthentication(ForumSubscription*,QAuthenticator*)));
    connect(ue, SIGNAL(getForumAuthentication(ForumSubscription*)), this, SLOT(getForumAuthentication(ForumSubscription*)));
    connect(ue, SIGNAL(loginFinished(ForumSubscription*,bool)), this, SLOT(forumLoginFinished(ForumSubscription*,bool)));
    connect(ue, SIGNAL(stateChanged(UpdateEngine*, UpdateEngine::UpdateEngineState, UpdateEngine::UpdateEngineState)),
            this, SLOT(updateEngineStateChanged(UpdateEngine*, UpdateEngine::UpdateEngineState, UpdateEngine::UpdateEngineState)));
    connect(ue, SIGNAL(updateForumSubscription(ForumSubscription *)), &m_protocol, SLOT(subscribeForum(ForumSubscription *)));
    connect(ue, SIGNAL(messagePosted(ForumSubscription*)), this, SLOT(updateClicked(ForumSubscription*)));
}

void ClientLogic::subscriptionDeleted(QObject* subobj) {
    ForumSubscription *sub = static_cast<ForumSubscription*> (subobj);
    if(!engines.contains(sub)) return; // Possible when quitting
    busyUpdateEngines.remove(engines[sub]);
    // engines[sub]->cancelOperation(); Don't do this
    engines[sub]->setSubscription(0);
    engines[sub]->deleteLater();
    engines.remove(sub);
    subscriptionsNotUpdated.remove(sub);
}

void ClientLogic::forumUpdated(ForumSubscription* forum) {
    if(forum->errorList().size()) {
        m_settings->setUpdateFailed(forum->id(), true);
    }
    int busyForums = busyForumCount();

    subscriptionsNotUpdated.remove(forum);
    subscriptionsToUpdateLeft.removeAll(forum);
    ForumSubscription *nextSub = 0;
    while(!nextSub && !subscriptionsToUpdateLeft.isEmpty()
          && busyForums <= MAX_CONCURRENT_UPDATES) {
        nextSub = subscriptionsToUpdateLeft.takeFirst();
        if(m_forumDatabase.contains(nextSub)) {
            nextSub->setScheduledForUpdate(false);
            Q_ASSERT(!nextSub->beingSynced());
            Q_ASSERT(!nextSub->scheduledForSync());
            nextSub->updateEngine()->updateForum();
            busyForums++;
        }
    }
}

void ClientLogic::userSettingsReceived(bool success, UserSettings *newSettings) {
    if (!success) {
        errorDialog("Getting settings failed. Please check network connection.");
    } else {
        usettings.setSyncEnabled(newSettings->syncEnabled());
        m_settings->setSyncEnabled(usettings.syncEnabled());
        m_settings->sync();
        settingsChanged(false);
    }
}

void ClientLogic::moreMessagesRequested(ForumThread* thread) {
    Q_ASSERT(thread);
    UpdateEngine *engine = engines.value(thread->group()->subscription());
    Q_ASSERT(engine);
    if(engine->state() != UpdateEngine::UES_IDLE) return;
    if(thread->group()->subscription()->beingSynced()) return;
    if(thread->group()->subscription()->scheduledForSync()) return;

    thread->setGetMessagesCount(thread->getMessagesCount() + m_settings->showMoreCount());
    thread->commitChanges();
    engine->updateThread(thread);
}

void ClientLogic::unsubscribeGroup(ForumGroup *group) {
    group->setSubscribed(false);
    group->commitChanges();
    if(!noAccount())
        m_protocol.subscribeGroups(group->subscription());
}

void ClientLogic::forumUpdateNeeded(ForumSubscription *fs) {
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT(fs);

    if(!noAccount())
        m_protocol.subscribeForum(fs); // Resend the forum infos
}

void ClientLogic::unregisterSiilihai() {
    cancelClicked();
    usettings.setSyncEnabled(false);
    dbStored = true;
    haltSiilihai();
    m_forumDatabase.resetDatabase();
    m_forumDatabase.storeDatabase();
    m_settings->clear();
    m_settings->sync();
}

// Called when app about to quit - handle upsync & quitting
void ClientLogic::aboutToQuit() {
    qDebug() << Q_FUNC_INFO;
    //    QEventLoop eventLoop;
    //    eventLoop.exec();
}

void ClientLogic::setDeveloperMode(bool newDm) {
    if (devMode != newDm) {
        devMode = newDm;
        emit developerModeChanged(newDm);
    }
}

void ClientLogic::databaseStored() {
    if(currentState==SH_STOREDB)
        haltSiilihai();
}

// Caution - engine->subscription() may be null (when deleted)!
void ClientLogic::updateEngineStateChanged(UpdateEngine *engine, UpdateEngine::UpdateEngineState newState, UpdateEngine::UpdateEngineState oldState) {
    if (newState==UpdateEngine::UES_UPDATING) {
        busyUpdateEngines.insert(engine);
    } else {
        busyUpdateEngines.remove(engine);
    }
    if (!busyUpdateEngines.isEmpty()) {
        showStatusMessage("Updating Forums.. ");
    }
    if(currentState==SH_READY && newState == UpdateEngine::UES_IDLE && oldState != UpdateEngine::UES_UPDATING) {
        updateClicked(engine->subscription());
    }
}

void ClientLogic::loginWizardFinished() {
    if (m_settings->username().length() == 0 && !noAccount()) {
        haltSiilihai();
    } else {
        showMainWindow();
        settingsChanged(false);
        loginFinished(true, QString(), usettings.syncEnabled());
    }
}

bool ClientLogic::noAccount() {
    return m_settings->noAccount();
}

void ClientLogic::subscriptionFound(ForumSubscription *sub) {
    connect(sub, SIGNAL(destroyed(QObject*)), this, SLOT(subscriptionDeleted(QObject*)));
    subscriptionsNotUpdated.insert(sub);
    createEngineForSubscription(sub);
}

void ClientLogic::updateGroupSubscriptions(ForumSubscription *sub) {
    if(!noAccount())
        m_protocol.subscribeGroups(sub);
    updateForum(sub);
}

void ClientLogic::updateAllParsers() {
    if(currentState != SH_READY) return;
    foreach(UpdateEngine *eng, engines) {
        if(eng->state()==UpdateEngine::UES_IDLE && eng->subscription()->provider() == ForumSubscription::FP_PARSER) {
            ForumSubscriptionParsed *subParser = qobject_cast<ForumSubscriptionParsed*> (eng->subscription());
            Q_ASSERT(subParser);
            m_parserManager->updateParser(subParser->parserId());
        }
    }
}

void ClientLogic::updateThread(ForumThread* thread, bool force) {
    if(currentState != SH_READY) return;
    qDebug() << Q_FUNC_INFO << thread->toString() << force;
    ForumSubscription *sub = thread->group()->subscription();
    Q_ASSERT(sub);
    Q_ASSERT(engines.contains(sub));

    // Are we too strict? Show dialog if not possible yet?
    if(thread->group()->subscription()->beingSynced()) return;
    if(thread->group()->subscription()->scheduledForSync()) return;
    if(thread->group()->subscription()->scheduledForUpdate()) return;
    if(sub->updateEngine()->state() != UpdateEngine::UES_IDLE) return;

    engines.value(sub)->updateThread(thread, force);
}

void ClientLogic::showStatusMessage(QString message) {
    if (statusMsg != message) {
        statusMsg = message;
        emit statusMessageChanged(message);
        if(!message.isEmpty()) statusMsgTimer.start(5000);
    }
}

void ClientLogic::forumLoginFinished(ForumSubscription *sub, bool success) {
    if(!success)
        errorDialog(QString("Login to %1 failed. Please check credentials.").arg(sub->alias()));
}

// Authenticator can be null!
void ClientLogic::getHttpAuthentication(ForumSubscription *fsub, QAuthenticator *authenticator) {
    qDebug() << Q_FUNC_INFO << fsub->alias();
    bool failed = m_settings->updateFailed(fsub->id());
    qDebug() << Q_FUNC_INFO << "Failed:" << failed;
    QString gname = QString().number(fsub->id());
    // Must exist and be longer than zero
    if(!failed &&
            m_settings->contains(QString("authentication/%1/username").arg(gname)) &&
            m_settings->value(QString("authentication/%1/username").arg(gname)).toString().length() > 0) {
        if(authenticator && !failed) {
            qDebug() << Q_FUNC_INFO << "reading u/p from settings";
            authenticator->setUser(m_settings->value(QString("authentication/%1/username").arg(gname)).toString());
            authenticator->setPassword(m_settings->value(QString("authentication/%1/password").arg(gname)).toString());
        }
    }
    if(!authenticator || authenticator->user().isNull() || failed) { // Ask user the credentials
        qDebug() << Q_FUNC_INFO << "asking user for http credentials";
        // Return empty authenticator
        authenticator->setUser(QString::null);
        authenticator->setPassword(QString::null);

        CredentialsRequest *cr = new CredentialsRequest(this);
        cr->subscription = fsub;
        cr->credentialType = CredentialsRequest::SH_CREDENTIAL_HTTP;
        credentialsRequests.append(cr);
        m_settings->setUpdateFailed(fsub->id(), false); // Reset failed status
        if(!currentCredentialsRequest)
            showNextCredentialsDialog();
    }
}

void ClientLogic::getForumAuthentication(ForumSubscription *fsub) {
    qDebug() << Q_FUNC_INFO << fsub->alias();
    CredentialsRequest *cr = new CredentialsRequest(this);
    cr->subscription = fsub;
    cr->credentialType = CredentialsRequest::SH_CREDENTIAL_FORUM;
    credentialsRequests.append(cr);
    if(!currentCredentialsRequest)
        showNextCredentialsDialog();
}

void ClientLogic::showNextCredentialsDialog() {
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT(!currentCredentialsRequest);
    if(credentialsRequests.isEmpty()) return;
    currentCredentialsRequest = credentialsRequests.takeFirst();
    connect(currentCredentialsRequest, SIGNAL(credentialsEntered(bool)), this, SLOT(credentialsEntered(bool)));
    showCredentialsDialog();
}


void ClientLogic::credentialsEntered(bool store) {
    CredentialsRequest *cr = qobject_cast<CredentialsRequest*>(sender());
    Q_ASSERT(cr==currentCredentialsRequest);
    Q_ASSERT(engines.value(currentCredentialsRequest->subscription));
    qDebug() << Q_FUNC_INFO << store << currentCredentialsRequest->authenticator.user();

    currentCredentialsRequest->subscription->setAuthenticated(!currentCredentialsRequest->authenticator.user().isEmpty());

    if(store) {
        if(cr->credentialType == CredentialsRequest::SH_CREDENTIAL_HTTP) {
            qDebug() << Q_FUNC_INFO << "storing into settings";
            m_settings->beginGroup("authentication");
            m_settings->beginGroup(QString::number(currentCredentialsRequest->subscription->id()));
            m_settings->setValue("username", currentCredentialsRequest->authenticator.user());
            m_settings->setValue("password", currentCredentialsRequest->authenticator.password());
            m_settings->setValue("failed", "false");
            m_settings->endGroup();
            m_settings->endGroup();
            m_settings->sync();
        } else if(cr->credentialType == CredentialsRequest::SH_CREDENTIAL_FORUM) {
            qDebug() << Q_FUNC_INFO << "storing into subscription";
            currentCredentialsRequest->subscription->setUsername(currentCredentialsRequest->authenticator.user());
            currentCredentialsRequest->subscription->setPassword(currentCredentialsRequest->authenticator.password());
        }
    }
    // @todo move to SiilihaiSettings!!
    if(!currentCredentialsRequest->subscription->isAuthenticated()) {
        // Remove authentication
        m_settings->beginGroup("authentication");
        m_settings->remove(QString::number(currentCredentialsRequest->subscription->id()));
        m_settings->endGroup();

        currentCredentialsRequest->subscription->setUsername(QString::null);
        currentCredentialsRequest->subscription->setPassword(QString::null);
    }

    if(!noAccount())
        m_protocol.subscribeForum(currentCredentialsRequest->subscription); // Resend the forum infos

    UpdateEngine *engine = engines.value(currentCredentialsRequest->subscription);
    engine->credentialsEntered(currentCredentialsRequest);
    currentCredentialsRequest->deleteLater();
    currentCredentialsRequest=0;
    showNextCredentialsDialog();
}

void ClientLogic::syncProgress(float progress, QString message) {
    Q_UNUSED(progress);
    showStatusMessage(message);
}

void ClientLogic::unsubscribeForum(ForumSubscription* fs) {
    if(!noAccount())
        m_protocol.subscribeForum(fs, true);
    m_forumDatabase.deleteSubscription(fs);
    if(fs->provider() == ForumSubscription::FP_PARSER)
        m_parserManager->deleteParser(qobject_cast<ForumSubscriptionParsed*>(fs)->parserId());
}
