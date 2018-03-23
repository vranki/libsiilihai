#include "clientlogic.h"
#include <time.h>
#include <QDir>
#include <QNetworkProxy>
#include <QTimer>
#include <QNetworkConfigurationManager>
#include <QProcessEnvironment>
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

ClientLogic::ClientLogic(QObject *parent) : QObject(parent)
  , m_settings(nullptr)
  , m_forumDatabase(this)
  , m_syncmaster(this, m_forumDatabase, m_protocol)
  , m_parserManager(nullptr)
  , m_currentCredentialsRequest(nullptr)
  , currentState(SH_OFFLINE)
  , m_subscriptionManagement(nullptr)
  , endSyncDone(false)
  , firstRun(true)
  , devMode(false)
  , dbStored(false)
{
    QNetworkConfigurationManager ncm;
    QNetworkConfiguration config = ncm.defaultConfiguration();
    networkSession = new QNetworkSession(config, this);
    qDebug() << Q_FUNC_INFO << "Network connected:" << (networkSession->state() == QNetworkSession::Connected);

    statusMsgTimer.setSingleShot(true);

    connect(&statusMsgTimer, SIGNAL(timeout()), this, SLOT(clearStatusMessage()));

    // Make sure Siilihai::subscriptionFound is called first to get ParserEngine
    connect(&m_forumDatabase, &ForumDatabase::subscriptionFound, this, &ClientLogic::subscriptionFound);
    connect(&m_forumDatabase, &ForumDatabase::subscriptionRemoved, this, &ClientLogic::subscriptionRemoved);
    connect(&m_forumDatabase, SIGNAL(databaseStored()), this, SLOT(databaseStored()), Qt::QueuedConnection);
    connect(&m_protocol, &SiilihaiProtocol::userSettingsReceived, this, &ClientLogic::userSettingsReceived);
    connect(&m_protocol, &SiilihaiProtocol::loginFinished, this, &ClientLogic::loginFinishedSlot);
    connect(&m_protocol, &SiilihaiProtocol::networkError, this, &ClientLogic::protocolError);
    connect(this, &ClientLogic::offlineChanged, &m_protocol, &SiilihaiProtocol::setOffline);
    connect(&m_syncmaster, &SyncMaster::syncProgress, this, &ClientLogic::syncProgress);
    connect(&m_syncmaster, &SyncMaster::syncFinished, this, &ClientLogic::syncFinished);
}

ClientLogic::~ClientLogic()
{
    qDebug() << Q_FUNC_INFO;
    m_forumDatabase.resetDatabase();
}

QString ClientLogic::statusMessage() const
{
    return statusMsg;
}

ClientLogic::SiilihaiState ClientLogic::state() const {
    return currentState;
}

bool ClientLogic::developerMode() const {
    return devMode;
}

QString ClientLogic::addReToSubject(QString subject) {
    return subject.startsWith("Re:") ? subject : "Re: " + subject;
}

QString ClientLogic::addQuotesToBody(QString body) {
    body = MessageFormatting::stripHtml(body);
    body = "[quote]\n" + body + "\n[/quote]\n\n";
    return body;
}

void ClientLogic::launchSiilihai() {
    m_settings = createSettings();
    qDebug() << Q_FUNC_INFO << "Loaded settings from " << m_settings->fileName();
    emit settingsChangedSignal(m_settings);

    // Create SM only after creating settings, as it needs it.
    m_subscriptionManagement = new SubscriptionManagement(nullptr, &m_protocol, m_settings);
    connect(m_subscriptionManagement, &SubscriptionManagement::forumUnsubscribed, this, &ClientLogic::unsubscribeForum);
    connect(m_subscriptionManagement, &SubscriptionManagement::showError, this, &ClientLogic::appendMessage);
    connect(m_subscriptionManagement, &SubscriptionManagement::forumAdded, this, &ClientLogic::forumAdded);
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
            appendMessage("Warning: http proxy is not valid URL");
        }
    }

    QDir dataDir(getDataFilePath());
    if(!dataDir.exists()) dataDir.mkpath(getDataFilePath());

    m_parserManager = new ParserManager(this, &m_protocol);
    m_parserManager->openDatabase(getDataFilePath() + "/siilihai_parsers.xml");

    settingsChanged(false);

    // Set SIILIHAI_SERVER env variable to override base url
    QString envBaseUrl = QProcessEnvironment::systemEnvironment().value("SIILIHAI_SERVER");
    m_protocol.setBaseURL(envBaseUrl.isEmpty() ? m_settings->baseUrl() : envBaseUrl);

    QString databaseFileName = getDataFilePath() + "/siilihai_forums.xml";

    qDebug() << Q_FUNC_INFO << "Using data files under " << getDataFilePath();

    if(firstRun) {
        m_forumDatabase.openDatabase(databaseFileName); // Fails
    } else {
        int currentSchemaVersion = m_settings->databaseSchema();
        if(m_forumDatabase.schemaVersion() != currentSchemaVersion) {
            appendMessage("The database schema has been changed. Your forum database will be reset. Sorry. ");
            m_forumDatabase.openDatabase(databaseFileName, false);
        } else {
            if(!m_forumDatabase.openDatabase(databaseFileName)) {
                appendMessage("Could not open Siilihai's forum database file.\n"
                              "See console for details.");
            }
        }
        if(!m_settings->cleanShutdown()) {
            appendMessage("Siilihai was not able to shut down cleanly last time.\n"
                          "Sorry if it crashed.");
        }
    }
    m_settings->setDatabaseSchema(m_forumDatabase.schemaVersion());
    m_settings->setCleanShutdown(false);
    m_settings->sync();

    if(networkSession->state() != QNetworkSession::Connected)
        setOffline(true);

    if (m_settings->username().isEmpty() && !noAccount()) {
        emit showLoginWizard();
    } else {
        showMainWindow();
        tryLogin();
    }
}

void ClientLogic::haltSiilihai() {
    showStatusMessage("Quitting Siilihai.. ");
    cancelClicked();

    for(auto engine : engines.values()) {
        engine->subscription()->engineDestroyed();
        engine->setSubscription(nullptr);
        engine->deleteLater();
    }

    engines.clear();
    if(usettings.syncEnabled() && !endSyncDone && currentState == SH_READY) {
        qDebug() << "Sync enabled - running end sync";
        setState(SH_ENDSYNC);
    } else {
        if(currentState != SH_STOREDB && !dbStored) {
            setState(SH_STOREDB);
        } else {
            qDebug() << Q_FUNC_INFO << "All done - quitting";
            m_settings->setCleanShutdown(true);
            emit closeUi();
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

void ClientLogic::getMoreMessages(ForumThread *thread) {
    Q_ASSERT(thread);
    UpdateEngine *ue = thread->group()->subscription()->updateEngine();
    if(ue->state() == UpdateEngine::UES_IDLE) {
        thread->setGetMessagesCount(thread->getMessagesCount() + GET_MORE_MESSAGES_COUNT);
        ue->updateThread(thread, false, true);
    } else {
        qWarning() << Q_FUNC_INFO << "Update engine NOT idle, ignoring this..";
    }
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

CredentialsRequest *ClientLogic::currentCredentialsRequest() const
{
    return m_currentCredentialsRequest;
}

bool ClientLogic::offline() const
{
    return state() == SH_OFFLINE;
}

void ClientLogic::settingsChanged(bool byUser) {
    usettings.setSyncEnabled(m_settings->syncEnabled());
    if(byUser && !noAccount()) {
        m_protocol.setUserSettings(&usettings);
    }
    m_settings->sync();
    if(usettings.syncEnabled()) { // Force upsync of everything
        for(ForumSubscription *sub : m_forumDatabase) {
            for(auto group : *sub) {
                group->setHasChanged(true);
            }
        }
    }
}

void ClientLogic::loginUser(QString user, QString password) {
    qDebug() << Q_FUNC_INFO << user;

    m_settings->setUsername(user.trimmed());
    m_settings->setPassword(password.trimmed());

    tryLogin();
}

// U & P must be in settings.
void ClientLogic::tryLogin() {
    Q_ASSERT(currentState==SH_OFFLINE);
    if(noAccount()) {
        QTimer::singleShot(1, this, SLOT(accountlessLoginFinished()));
        return;
    }
    setState(SH_LOGIN);
    if(networkSession->state() == QNetworkSession::Connected) {
        m_protocol.login(m_settings->username(), m_settings->password());
    } else {
        emit loginFinished(false, "Offline mode", usettings.syncEnabled());
        setState(SH_OFFLINE);
    }
}

void ClientLogic::accountlessLoginFinished() {
    loginFinishedSlot(true, "", false);
}

void ClientLogic::loginFinishedSlot(bool success, QString motd, bool sync) {
    if (success) {
        usettings.setSyncEnabled(sync);
        m_settings->setSyncEnabled(usettings.syncEnabled());
        m_settings->sync();
        setState((usettings.syncEnabled() && timeForUpdate()) ? SH_STARTSYNCING : SH_READY);
    } else {
        appendMessage("Login failed. \n" + motd);
        // @todo this happens also on first run if logging in with wrong u/p.
        // using login as network test sucks anyway.
        setState(SH_OFFLINE);
    }

    if(!success && !m_settings->username().isEmpty()) {
        // Not success & we have registered.. Network or server down
        // This happens when network is down & can't login.
        // DON'T show login wizard.
        // emit showLoginWizard(); NO
        setState(SH_OFFLINE);
    }

    // And then emit loginFinished to show the error..
    emit loginFinished(success, motd, sync);
}

void ClientLogic::clearStatusMessage() {
    showStatusMessage();
}

void ClientLogic::protocolError(QString message)
{
    appendMessage(message);
    setState(SH_OFFLINE);
}

SiilihaiSettings *ClientLogic::createSettings() {
    return new SiilihaiSettings(getDataFilePath() + "/siilihai_settings.ini", QSettings::IniFormat, this);
}

void ClientLogic::setState(SiilihaiState newState) {
    if(newState == currentState) {
        qDebug() << Q_FUNC_INFO << "Already in state " << newState;
        return;
    }

    SiilihaiState previousState = currentState;
    currentState = newState;

    if(previousState==SH_OFFLINE || newState==SH_OFFLINE) {
        qDebug() << Q_FUNC_INFO << "Offline changed: " << (newState==SH_OFFLINE);
        emit offlineChanged(newState==SH_OFFLINE);
    }

    if(newState==SH_OFFLINE) {
        qDebug() << Q_FUNC_INFO << "Offline";
        networkSession->close();
        Q_ASSERT(previousState==SH_LOGIN
                 || previousState==SH_STARTSYNCING
                 || previousState==SH_READY
                 || previousState==SH_ENDSYNC);
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
            m_syncmaster.startSync();
        }
    } else if(newState==SH_ENDSYNC) {
        qDebug() << Q_FUNC_INFO << "Endsync";
        showStatusMessage("Synchronizing with server..");
        m_syncmaster.endSync();
    } else if(newState==SH_STOREDB) {
        qDebug() << Q_FUNC_INFO << "Storedb";
        saveData();
        dbStored = true;
    } else if(newState==SH_READY) {
        qDebug() << Q_FUNC_INFO << "Ready";

        if(timeForUpdate()) updateClicked();

        if (m_forumDatabase.isEmpty()) { // Display subscribe dialog if none subscribed
            emit showSubscribeForumDialog();
        }
    }
    emit stateChanged(currentState);
}

void ClientLogic::updateForum(ForumSubscription *sub) {
    if(!engines.contains(sub)) return; // Can happen if quitting

    subscriptionsNotUpdated.remove(sub);

    if(sub->beingUpdated()) return;

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
        qDebug() << Q_FUNC_INFO << "updating now " << sub->toString() << subscriptionsToUpdateLeft;
        engines.value(sub)->updateForum();
    } else {
        Q_ASSERT(!sub->beingSynced());
        qDebug() << Q_FUNC_INFO << "updating later " << sub->toString() << busyForums;
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
                && currentState != SH_OFFLINE) {
            sub->setScheduledForUpdate(false);
            subscriptionsToUpdateLeft.removeAll(sub);
            subscriptionsNotUpdated.remove(sub);
            Q_ASSERT(!engine->subscription()->beingSynced());
            engine->updateForum(force);
        }
    } else { // Update all
        for(auto engine : engines.values()) {
            updateForum(engine->subscription());
        }
    }
}

void ClientLogic::cancelClicked() {
    for(auto sub : m_forumDatabase) {
        sub->setScheduledForUpdate(false);
    }
    for(auto engine : engines.values()) {
        engine->cancelOperation();
    }
    if(currentState==SH_STARTSYNCING || currentState==SH_ENDSYNC) {
        m_syncmaster.cancel();
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
    if (!success) {
        appendMessage(QString("Syncing status with server failed.\n\n%1").arg(message));
    }
    if(currentState == SH_STARTSYNCING) {
        setState(SH_READY);
    } else if(currentState == SH_ENDSYNC) {
        endSyncDone = true;
        haltSiilihai();
    }
}

void ClientLogic::listSubscriptionsFinished(QList<int> serversSubscriptions) {
    disconnect(&m_protocol, SIGNAL(listSubscriptionsFinished(QList<int>)), this, SLOT(listSubscriptionsFinished(QList<int>)));

    QList<ForumSubscription*> unsubscribedForums;
    for(ForumSubscription* sub : m_forumDatabase) {
        bool found = false;
        if(sub->provider() == ForumSubscription::FP_PARSER) {
            for(int serverSubscriptionId : serversSubscriptions) {
                if(serverSubscriptionId == qobject_cast<ForumSubscriptionParsed*>(sub)->parserId())
                    found = true;
            }
            if (!found) {
                unsubscribedForums.append(sub);
            }
        }
    }
    for(auto sub : unsubscribedForums) {
        if(sub->provider() == ForumSubscription::FP_PARSER) {
            m_parserManager->deleteParser(qobject_cast<ForumSubscriptionParsed*>(sub)->parserId());
        }
        if(sub->provider() == ForumSubscription::FP_PARSER) // @todo not tapatalk yet!
            m_forumDatabase.deleteSubscription(sub);
    }
}



// Note: fs *MUST* be a real ForumSubscription derived class, NOT just ForumSubscription with provider set!
void ClientLogic::forumAdded(ForumSubscription *fs) {
    Q_ASSERT(fs->id());

    if(m_forumDatabase.contains(fs->id())) {
        appendMessage("You have already subscribed to " + fs->alias());
    } else {
        ForumSubscription *newFs = ForumSubscription::newForProvider(fs->provider(), &m_forumDatabase, false);

        Q_ASSERT(newFs);
        newFs->copyFrom(fs);
        if(!m_forumDatabase.addSubscription(newFs)) { // Emits subscriptionFound
            appendMessage("Error: Unable to subscribe to forum. Check the log.");
            return;
        }
        fs = nullptr;
        createEngineForSubscription(newFs);
        Q_ASSERT(engines.value(newFs));
        engines.value(newFs)->updateGroupList();
        if(!noAccount()) m_protocol.subscribeForum(newFs);
        emit forumSubscribed(newFs);
    }
}

void ClientLogic::createEngineForSubscription(ForumSubscription *newFs) {
    Q_ASSERT(!newFs->isTemp());
    if(engines.contains(newFs)) return;
    UpdateEngine *ue = UpdateEngine::newForSubscription(newFs, &m_forumDatabase, m_parserManager);
    Q_ASSERT(!engines.contains(newFs));
    engines.insert(newFs, ue);
    ue->setSubscription(newFs);
    connect(ue, &UpdateEngine::groupListChanged, this, &ClientLogic::groupListChanged);
    connect(ue, SIGNAL(forumUpdated(ForumSubscription*)), this, SLOT(forumUpdated(ForumSubscription*)));
    // Overloaded signal, watch out when changing to new style connection
    connect(ue, SIGNAL(getHttpAuthentication(ForumSubscription*,QAuthenticator*)), this, SLOT(getHttpAuthentication(ForumSubscription*,QAuthenticator*)));
    connect(ue, SIGNAL(getForumAuthentication(ForumSubscription*)), this, SLOT(getForumAuthentication(ForumSubscription*)));
    connect(ue, SIGNAL(loginFinished(ForumSubscription*,bool)), this, SLOT(forumLoginFinished(ForumSubscription*,bool)));
    connect(ue, SIGNAL(stateChanged(UpdateEngine*, UpdateEngine::UpdateEngineState, UpdateEngine::UpdateEngineState)),
            this, SLOT(updateEngineStateChanged(UpdateEngine*, UpdateEngine::UpdateEngineState, UpdateEngine::UpdateEngineState)));
    connect(ue, SIGNAL(updateForumSubscription(ForumSubscription *)), &m_protocol, SLOT(subscribeForum(ForumSubscription *)));
    connect(ue, SIGNAL(messagePosted(ForumSubscription*)), this, SLOT(updateClicked(ForumSubscription*)));
}

void ClientLogic::subscriptionRemoved(ForumSubscription *sub) {
    if(!engines.contains(sub)) return; // Possible when quitting
    busyUpdateEngines.remove(engines[sub]);
    subscriptionsNotUpdated.remove(sub);
    // engines[sub]->cancelOperation(); Don't do this
    engines[sub]->setSubscription(nullptr);
    engines[sub]->deleteLater();
    engines.remove(sub);
}

void ClientLogic::forumUpdated(ForumSubscription* forum) {
    if(forum->errorList().size()) {
        m_settings->setUpdateFailed(forum->id(), true);
    }
    int busyForums = busyForumCount();

    subscriptionsNotUpdated.remove(forum);
    subscriptionsToUpdateLeft.removeAll(forum);
    ForumSubscription *nextSub = nullptr;
    while(!subscriptionsToUpdateLeft.isEmpty()
          && busyForums <= MAX_CONCURRENT_UPDATES) {
        nextSub = subscriptionsToUpdateLeft.takeFirst();
        if(m_forumDatabase.contains(nextSub)) {
            nextSub->setScheduledForUpdate(false);
            Q_ASSERT(!nextSub->beingSynced());
            Q_ASSERT(!nextSub->scheduledForSync());
            qDebug() << Q_FUNC_INFO << "Now updating " << nextSub->alias() << busyForums;
            nextSub->updateEngine()->updateForum();
            busyForums++;
        }
    }
    if(subscriptionsToUpdateLeft.isEmpty()) {
        m_settings->setLastUpdate(QDateTime::currentDateTime());
    }
}

void ClientLogic::userSettingsReceived(bool success, UserSettings *newSettings) {
    if (!success) {
        appendMessage("Getting settings failed. Please check network connection.");
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
    engine->updateThread(thread, false, true);
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
        // Login finished should be already done here..
        if(noAccount()) setState(SH_READY);
    }
}

void ClientLogic::setOffline(bool newOffline)
{
    if (offline() == newOffline)
        return;

    qDebug() << Q_FUNC_INFO << newOffline;
    if(newOffline && currentState == SH_READY) {
        setState(SH_OFFLINE);
    } else if(!newOffline && currentState == SH_OFFLINE) {
        tryLogin();
    }
    emit offlineChanged(newOffline);
}

void ClientLogic::saveData()
{
    qDebug() << Q_FUNC_INFO;
    if (m_settings) m_settings->sync();
    if(!m_forumDatabase.storeDatabase()) {
        appendMessage("Failed to save forum database file");
    }
}

bool ClientLogic::noAccount() {
    return m_settings->noAccount();
}

bool ClientLogic::timeForUpdate()
{
    QDateTime lastUpdate = m_settings->lastUpdate();
    if(!lastUpdate.isValid())
        return true;
    QDateTime now = QDateTime::currentDateTime();
    qDebug() << Q_FUNC_INFO << "Last update was " << lastUpdate.secsTo(now) / 60
             << "minutes ago";
    return lastUpdate.secsTo(now) > 30 * 60; // 30 mins..
}

void ClientLogic::subscriptionFound(ForumSubscription *sub) {
    subscriptionsNotUpdated.insert(sub);
    createEngineForSubscription(sub);
}

void ClientLogic::updateGroupSubscriptions(ForumSubscription *sub) {
    emit sub->groupsChanged();
    if(!noAccount()) m_protocol.subscribeGroups(sub);
    updateForum(sub);
}

void ClientLogic::updateAllParsers() {
    if(currentState != SH_READY) return;
    for(auto eng : engines) {
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
        appendMessage(QString("Login to %1 failed. Please check credentials.").arg(sub->alias()));
}

// Authenticator can be null!
void ClientLogic::getHttpAuthentication(ForumSubscription *fsub, QAuthenticator *authenticator) {
    bool failed = m_settings->updateFailed(fsub->id());
    QString gname = QString().number(fsub->id());
    // Must exist and be longer than zero
    if(!failed &&
            m_settings->contains(QString("authentication/%1/username").arg(gname)) &&
            m_settings->value(QString("authentication/%1/username").arg(gname)).toString().length() > 0) {
        if(authenticator && !failed) {
            authenticator->setUser(m_settings->value(QString("authentication/%1/username").arg(gname)).toString());
            authenticator->setPassword(m_settings->value(QString("authentication/%1/password").arg(gname)).toString());
        }
    }
    if(!authenticator || authenticator->user().isNull() || failed) { // Ask user the credentials
        // Return empty authenticator
        if(authenticator) {
            authenticator->setUser(QString());
            authenticator->setPassword(QString());
        }
        CredentialsRequest *cr = new CredentialsRequest(fsub, this);
        cr->credentialType = CredentialsRequest::SH_CREDENTIAL_HTTP;
        credentialsRequests.append(cr);
        m_settings->setUpdateFailed(fsub->id(), false); // Reset failed status
        if(!m_currentCredentialsRequest) showNextCredentialsDialog();
    }
}

void ClientLogic::getForumAuthentication(ForumSubscription *fsub) {
    CredentialsRequest *cr = new CredentialsRequest(fsub, this);
    cr->credentialType = CredentialsRequest::SH_CREDENTIAL_FORUM;
    credentialsRequests.append(cr);
    if(!m_currentCredentialsRequest) showNextCredentialsDialog();
}

void ClientLogic::showNextCredentialsDialog() {
    Q_ASSERT(!m_currentCredentialsRequest);
    if(credentialsRequests.isEmpty()) return;
    m_currentCredentialsRequest = credentialsRequests.takeFirst();
    connect(m_currentCredentialsRequest, &CredentialsRequest::credentialsEntered, this, &ClientLogic::credentialsEntered);
    emit currentCredentialsRequestChanged(m_currentCredentialsRequest);
}


void ClientLogic::credentialsEntered(bool store) {
    CredentialsRequest *cr = qobject_cast<CredentialsRequest*>(sender());
    Q_ASSERT(cr==m_currentCredentialsRequest);
    Q_ASSERT(engines.value(m_currentCredentialsRequest->subscription()));

    m_currentCredentialsRequest->subscription()->setAuthenticated(!m_currentCredentialsRequest->username().isEmpty());

    if(store) {
        if(cr->credentialType == CredentialsRequest::SH_CREDENTIAL_HTTP) {
            m_settings->beginGroup("authentication");
            m_settings->beginGroup(QString::number(m_currentCredentialsRequest->subscription()->id()));
            m_settings->setValue("username", m_currentCredentialsRequest->username());
            m_settings->setValue("password", m_currentCredentialsRequest->password());
            m_settings->setValue("failed", "false");
            m_settings->endGroup();
            m_settings->endGroup();
            m_settings->sync();
        } else if(cr->credentialType == CredentialsRequest::SH_CREDENTIAL_FORUM) {
            m_currentCredentialsRequest->subscription()->setUsername(m_currentCredentialsRequest->username());
            m_currentCredentialsRequest->subscription()->setPassword(m_currentCredentialsRequest->password());
        }
    }
    // @todo move to SiilihaiSettings!!
    if(!m_currentCredentialsRequest->subscription()->isAuthenticated()) {
        // Remove authentication
        m_settings->beginGroup("authentication");
        m_settings->remove(QString::number(m_currentCredentialsRequest->subscription()->id()));
        m_settings->endGroup();

        m_currentCredentialsRequest->subscription()->setUsername(QString());
        m_currentCredentialsRequest->subscription()->setPassword(QString());
    }

    if(!noAccount())
        m_protocol.subscribeForum(m_currentCredentialsRequest->subscription()); // Resend the forum infos

    UpdateEngine *engine = engines.value(m_currentCredentialsRequest->subscription());
    engine->credentialsEntered(m_currentCredentialsRequest);
    m_currentCredentialsRequest->deleteLater();
    m_currentCredentialsRequest = nullptr;
    emit currentCredentialsRequestChanged(m_currentCredentialsRequest);
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


void ClientLogic::appendMessage(QString message) {
    qDebug() << Q_FUNC_INFO << message;
    m_errorMessages.append(message);
    emit errorMessagesChanged(m_errorMessages);
}

void ClientLogic::dismissMessages() {
    m_errorMessages.clear();
    emit errorMessagesChanged(m_errorMessages);
}

QStringList ClientLogic::errorMessages() const {
    return m_errorMessages;
}
