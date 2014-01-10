#include "clientlogic.h"
#include <time.h>
#include <QDir>
#include <QNetworkProxy>
#include <QTimer>
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

ClientLogic::ClientLogic(QObject *parent) : QObject(parent), currentState(SH_OFFLINE), settings(0), forumDatabase(this), syncmaster(this, forumDatabase, protocol),
    parserManager(0), currentCredentialsRequest(0) {
    endSyncDone = false;
    firstRun = true;
    dbStored = false;
    srand ( time(NULL) );

    statusMsgTimer.setSingleShot(true);
    connect(&statusMsgTimer, SIGNAL(timeout()), this, SLOT(clearStatusMessage()));

    // Make sure Siilihai::subscriptionFound is called first to get ParserEngine
    connect(&forumDatabase, SIGNAL(subscriptionFound(ForumSubscription*)), this, SLOT(subscriptionFound(ForumSubscription*)));
    connect(&forumDatabase, SIGNAL(databaseStored()), this, SLOT(databaseStored()), Qt::QueuedConnection);
    connect(&protocol, SIGNAL(userSettingsReceived(bool,UserSettings*)), this, SLOT(userSettingsReceived(bool,UserSettings*)));
    connect(&syncmaster, SIGNAL(syncProgress(float, QString)), this, SLOT(syncProgress(float, QString)));
}

QString ClientLogic::statusMessage() const
{
    return statusMsg;
}

ClientLogic::siilihai_states ClientLogic::state() const {
    return currentState;
}

void ClientLogic::launchSiilihai(bool offline) {
    changeState(SH_STARTED);
    settings = createSettings();

    firstRun = settings->firstRun();
    settings->setFirstRun(false);

    QString proxy = settings->httpProxy();
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

    parserManager = new ParserManager(this, &protocol);
    parserManager->openDatabase(getDataFilePath() + "/siilihai_parsers.xml");

    settingsChanged(false);
    connect(&syncmaster, SIGNAL(syncFinished(bool, QString)), this, SLOT(syncFinished(bool, QString)));

    protocol.setBaseURL(settings->baseUrl());

    QString databaseFileName = getDataFilePath() + "/siilihai_forums.xml";

    qDebug() << Q_FUNC_INFO << "Using data files under " << getDataFilePath();

    if(firstRun) {
        forumDatabase.openDatabase(databaseFileName); // Fails
    } else {
        int currentSchemaVersion = settings->databaseSchema();
        if(forumDatabase.schemaVersion() != currentSchemaVersion) {
            errorDialog("The database schema has been changed. Your forum database will be reset. Sorry. ");
            forumDatabase.openDatabase(databaseFileName, false);
        } else {
            if(!forumDatabase.openDatabase(databaseFileName)) {
                errorDialog("Could not open Siilihai's forum database file.\n"
                            "See console for details.");
            }
        }
    }
    settings->setDatabaseSchema(forumDatabase.schemaVersion());
    settings->sync();

    if (settings->username().isEmpty() && !noAccount()) {
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
            settings->sync();
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

void ClientLogic::settingsChanged(bool byUser) {
    usettings.setSyncEnabled(settings->value("preferences/sync_enabled", false).toBool());
    if(byUser && !noAccount()) {
        protocol.setUserSettings(&usettings);
    }
    settings->sync();
    if(usettings.syncEnabled()) { // Force upsync of everything
        foreach(ForumSubscription *sub, forumDatabase.values()) {
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

    connect(&protocol, SIGNAL(loginFinished(bool, QString,bool)), this, SLOT(loginFinished(bool, QString,bool)));
    protocol.login(settings->username(), settings->password());
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
        Q_ASSERT(previousState==SH_LOGIN || previousState==SH_STARTSYNCING || previousState==SH_READY || previousState==SH_STARTED);
        if(previousState==SH_STARTSYNCING)
            syncmaster.cancel();
    } else if(newState==SH_LOGIN) {
        qDebug() << Q_FUNC_INFO << "Login";
        showStatusMessage("Logging in..");
    } else if(newState==SH_STARTSYNCING) {
        qDebug() << Q_FUNC_INFO << "Startsync";
        if(usettings.syncEnabled()) {
                foreach(ForumSubscription *sub, forumDatabase.values()) {
                    sub->setScheduledForUpdate(true);
                }
                connect(&syncmaster, SIGNAL(syncFinishedFor(ForumSubscription*)), this, SLOT(updateForum(ForumSubscription*)));
            syncmaster.startSync();
        }
    } else if(newState==SH_ENDSYNC) {
        qDebug() << Q_FUNC_INFO << "Endsync";
        showStatusMessage("Synchronizing with server..");
        disconnect(&syncmaster, SIGNAL(syncFinishedFor(ForumSubscription*)), this, SLOT(updateForum(ForumSubscription*)));
        syncmaster.endSync();
    } else if(newState==SH_STOREDB) {
        qDebug() << Q_FUNC_INFO << "Storedb";
        if(!forumDatabase.storeDatabase()) {
            errorDialog("Failed to save forum database file");
        }
        dbStored = true;
    } else if(newState==SH_READY) {
        qDebug() << Q_FUNC_INFO << "Ready";
        if(!usettings.syncEnabled())
            updateClicked();

        if (forumDatabase.isEmpty()) { // Display subscribe dialog if none subscribed
            subscribeForum();
        }
    }
}

void ClientLogic::updateForum(ForumSubscription *sub) {
    if(!engines.contains(sub)) return; // Can happen if quitting

    qDebug() << Q_FUNC_INFO << sub->toString();
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
    qDebug() << Q_FUNC_INFO << "Busy forums :" << busyForums << " max " << MAX_CONCURRENT_UPDATES;
    if(busyForums < MAX_CONCURRENT_UPDATES) {
        qDebug() << Q_FUNC_INFO << "Updating " << sub->toString();
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
    foreach(ForumSubscription *sub, forumDatabase.values()) {
        sub->setScheduledForUpdate(false);
    }
    foreach(UpdateEngine* engine, engines.values()) {
        engine->cancelOperation();
    }
}


int ClientLogic::busyForumCount() {
    int busyForums = 0;
    foreach(ForumSubscription *sub, forumDatabase.values()) {
        if(sub->updateEngine()->state()==UpdateEngine::UES_UPDATING) {
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
    disconnect(&protocol, SIGNAL(listSubscriptionsFinished(QList<int>)), this, SLOT(listSubscriptionsFinished(QList<int>)));

    QList<ForumSubscription*> unsubscribedForums;
    foreach(ForumSubscription* sub, forumDatabase.values()) {
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
        if(sub->isParsed()) {
            parserManager->deleteParser(qobject_cast<ForumSubscriptionParsed*>(sub)->parserId());
        }
        if(sub->isParsed()) // @todo not tapatalk yet!
            forumDatabase.deleteSubscription(sub);
    }
}


void ClientLogic::loginFinished(bool success, QString motd, bool sync) {
    disconnect(&protocol, SIGNAL(loginFinished(bool, QString,bool)), this, SLOT(loginFinished(bool, QString,bool)));

    if (success) {
        connect(&protocol, SIGNAL(listSubscriptionsFinished(QList<int>)), this, SLOT(listSubscriptionsFinished(QList<int>)));
        connect(&protocol, SIGNAL(sendParserReportFinished(bool)), this, SLOT(sendParserReportFinished(bool)));
        connect(&protocol, SIGNAL(subscribeForumFinished(ForumSubscription*, bool)), this, SLOT(subscribeForumFinished(ForumSubscription*,bool)));
        usettings.setSyncEnabled(sync);
        settings->setValue("preferences/sync_enabled", usettings.syncEnabled());
        settings->sync();
        if(usettings.syncEnabled()) {
            changeState(SH_STARTSYNCING);
        } else {
            changeState(SH_READY);
        }
    } else {
        errorDialog("Error: Login failed. Check your username, password and network connection.");
        // @todo this happens also on first run if logging in with wrong u/p.
        // using login as network test sucks anyway.

        // changeState(SH_OFFLINE);
    }
}

// Note: fs *MUST* be a real ForumSubscription derived class, NOT just ForumSubscription with provider set!
void ClientLogic::forumAdded(ForumSubscription *fs) {
    Q_ASSERT(fs->forumId());
    if(forumDatabase.contains(fs->forumId())) {
        errorDialog("You have already subscribed to " + fs->alias());
    } else {
        ForumSubscription *newFs = ForumSubscription::newForProvider(fs->provider(), &forumDatabase, false);

        Q_ASSERT(newFs);
        newFs->copyFrom(fs);
        if(!forumDatabase.addSubscription(newFs)) { // Emits subscriptionFound
            errorDialog("Error: Unable to subscribe to forum. Check the log.");
            return;
        }
        fs = 0;
        createEngineForSubscription(newFs);
        Q_ASSERT(engines.value(newFs));
        engines.value(newFs)->updateGroupList();
        if(!noAccount())
            protocol.subscribeForum(newFs);
    }
}

void ClientLogic::createEngineForSubscription(ForumSubscription *newFs) {
    Q_ASSERT(!newFs->isTemp());
    if(engines.contains(newFs)) return;
    UpdateEngine *ue = 0;
    if(newFs->isParsed()) {
        ForumSubscriptionParsed *newFsParsed = qobject_cast<ForumSubscriptionParsed*>(newFs);
        //        Q_ASSERT(parserManager->getParser(newFsParsed->parser())); // Should already be there!
        ParserEngine *pe = new ParserEngine(this, &forumDatabase, parserManager);
        ue = pe;
        pe->setParser(parserManager->getParser(newFsParsed->parserId()));
        if(!pe->parser()) pe->setParser(parserManager->getParser(newFsParsed->parserId())); // Load the (possibly old) parser
    } else if(newFs->isTapaTalk()) {
        ForumSubscriptionTapaTalk *newFsTt = qobject_cast<ForumSubscriptionTapaTalk*>(newFs);
        TapaTalkEngine *tte = new TapaTalkEngine(&forumDatabase, this);
        ue = tte;
    }
    Q_ASSERT(ue);
    Q_ASSERT(!engines.contains(newFs));
    engines.insert(newFs, ue);
    ue->setSubscription(newFs);
    connect(ue, SIGNAL(groupListChanged(ForumSubscription*)), this, SLOT(groupListChanged(ForumSubscription*)));
    connect(ue, SIGNAL(forumUpdated(ForumSubscription*)), this, SLOT(forumUpdated(ForumSubscription*)));
    connect(ue, SIGNAL(updateFailure(ForumSubscription*, QString)), this, SLOT(updateFailure(ForumSubscription*, QString)));
    connect(ue, SIGNAL(getHttpAuthentication(ForumSubscription*, QAuthenticator*)), this, SLOT(getHttpAuthentication(ForumSubscription*,QAuthenticator*)));
    connect(ue, SIGNAL(getForumAuthentication(ForumSubscription*)), this, SLOT(getForumAuthentication(ForumSubscription*)));
    connect(ue, SIGNAL(loginFinished(ForumSubscription*,bool)), this, SLOT(forumLoginFinished(ForumSubscription*,bool)));
    connect(ue, SIGNAL(stateChanged(UpdateEngine::UpdateEngineState, UpdateEngine::UpdateEngineState)),
            this, SLOT(parserEngineStateChanged(UpdateEngine::UpdateEngineState, UpdateEngine::UpdateEngineState)));
    connect(ue, SIGNAL(updateForumSubscription(ForumSubscription *)), &protocol, SLOT(subscribeForum(ForumSubscription *)));
    connect(ue, SIGNAL(messagePosted(ForumSubscription*)), this, SLOT(updateClicked(ForumSubscription*)));
}

void ClientLogic::subscriptionDeleted(QObject* subobj) {
    ForumSubscription *sub = static_cast<ForumSubscription*> (subobj);
    if(!engines.contains(sub)) return; // Possible when quitting
    busyParserEngines.remove(engines[sub]);
    // engines[sub]->cancelOperation(); Don't do this
    engines[sub]->setSubscription(0);
    engines[sub]->deleteLater();
    engines.remove(sub);
    subscriptionsNotUpdated.remove(sub);
}

void ClientLogic::forumUpdated(ForumSubscription* forum) {
    int busyForums = busyForumCount();

    subscriptionsNotUpdated.remove(forum);
    subscriptionsToUpdateLeft.removeAll(forum);
    ForumSubscription *nextSub = 0;
    while(!nextSub && !subscriptionsToUpdateLeft.isEmpty()
          && busyForums <= MAX_CONCURRENT_UPDATES) {
        nextSub = subscriptionsToUpdateLeft.takeFirst();
        if(forumDatabase.values().contains(nextSub)) {
            nextSub->setScheduledForUpdate(false);
            Q_ASSERT(!nextSub->beingSynced());
            Q_ASSERT(!nextSub->scheduledForSync());
            nextSub->updateEngine()->updateForum();
            busyForums++;
        }
    }
    if(!busyForums)
        forumDatabase.storeDatabase();
}

void ClientLogic::subscribeForumFinished(ForumSubscription *sub, bool success) {
    if (!success) {
        errorDialog("Subscribing to forum failed. Please check network connection.");
        if(forumDatabase.value(sub->forumId()))
            forumDatabase.deleteSubscription(sub);
    }
}

void ClientLogic::userSettingsReceived(bool success, UserSettings *newSettings) {
    if (!success) {
        errorDialog("Getting settings failed. Please check network connection.");
    } else {
        usettings.setSyncEnabled(newSettings->syncEnabled());
        settings->setValue("preferences/sync_enabled", usettings.syncEnabled());
        settings->sync();
        settingsChanged(false);
    }
}

void ClientLogic::updateFailure(ForumSubscription* fsub, QString msg) {
    QString key = QString("authentication/%1/failed").arg(fsub->forumId());
    qDebug() << Q_FUNC_INFO << "was: " << settings->value(key).toString();
    settings->setValue(key, "true");
    qDebug() << Q_FUNC_INFO << "now: " << settings->value(key).toString();
    errorDialog(fsub->alias() + "\n" + msg);
}

void ClientLogic::moreMessagesRequested(ForumThread* thread) {
    Q_ASSERT(thread);
    UpdateEngine *engine = engines.value(thread->group()->subscription());
    Q_ASSERT(engine);
    if(engine->state() != UpdateEngine::UES_IDLE) return;
    if(thread->group()->subscription()->beingSynced()) return;
    if(thread->group()->subscription()->scheduledForSync()) return;

    thread->setGetMessagesCount(thread->getMessagesCount() + settings->value("preferences/show_more_count", 30).toInt());
    thread->commitChanges();
    engine->updateThread(thread);
}

void ClientLogic::unsubscribeGroup(ForumGroup *group) {
    group->setSubscribed(false);
    group->commitChanges();
    if(!noAccount())
        protocol.subscribeGroups(group->subscription());
}

void ClientLogic::forumUpdateNeeded(ForumSubscription *fs) {
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT(fs);

    if(!noAccount())
        protocol.subscribeForum(fs); // Resend the forum infos
}

void ClientLogic::unregisterSiilihai() {
    cancelClicked();
    usettings.setSyncEnabled(false);
    dbStored = true;
    haltSiilihai();
    forumDatabase.resetDatabase();
    forumDatabase.storeDatabase();
    settings->clear();
    settings->sync();
}

// Called when app about to quit - handle upsync & quitting
void ClientLogic::aboutToQuit() {
    qDebug() << Q_FUNC_INFO;
//    QEventLoop eventLoop;
//    eventLoop.exec();
}

void ClientLogic::databaseStored() {
    if(currentState==SH_STOREDB)
        haltSiilihai();
}

// Caution - engine->subscription() may be null (when deleted)!
void ClientLogic::parserEngineStateChanged(UpdateEngine::UpdateEngineState newState, UpdateEngine::UpdateEngineState oldState) {
    ParserEngine *engine = qobject_cast<ParserEngine*>(sender());
    if (newState==UpdateEngine::UES_UPDATING) {
        busyParserEngines.insert(engine);
    } else {
        busyParserEngines.remove(engine);
    }
    if (!busyParserEngines.isEmpty()) {
        showStatusMessage("Updating Forums.. ");
    }
    if(currentState==SH_READY && newState == UpdateEngine::UES_IDLE && oldState != UpdateEngine::UES_UPDATING) {
        updateClicked(engine->subscription());
    }
}

void ClientLogic::loginWizardFinished() {
    if (settings->username().length() == 0 && !noAccount()) {
        haltSiilihai();
    } else {
        showMainWindow();
        settingsChanged(false);
        loginFinished(true, QString(), usettings.syncEnabled());
    }
}

bool ClientLogic::noAccount() {
    return settings->noAccount();
}

void ClientLogic::subscriptionFound(ForumSubscription *sub) {
    connect(sub, SIGNAL(destroyed(QObject*)), this, SLOT(subscriptionDeleted(QObject*)));
    subscriptionsNotUpdated.insert(sub);
    createEngineForSubscription(sub);
}

void ClientLogic::updateGroupSubscriptions(ForumSubscription *sub) {
    if(!noAccount())
        protocol.subscribeGroups(sub);
    updateForum(sub);
}

void ClientLogic::updateAllParsers() {
    if(currentState != SH_READY) return;
    foreach(UpdateEngine *eng, engines) {
        if(eng->state()==UpdateEngine::UES_IDLE && eng->subscription()->isParsed()) {
            ForumSubscriptionParsed *subParser = qobject_cast<ForumSubscriptionParsed*> (eng->subscription());
            Q_ASSERT(subParser);
            parserManager->updateParser(subParser->parserId());
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

void ClientLogic::unsubscribeForum(ForumSubscription* fs) {
    if(!noAccount())
        protocol.subscribeForum(fs, true);
    forumDatabase.deleteSubscription(fs);
    if(fs->isParsed())
        parserManager->deleteParser(qobject_cast<ForumSubscriptionParsed*>(fs)->parserId());
}

// Authenticator can be null!
void ClientLogic::getHttpAuthentication(ForumSubscription *fsub, QAuthenticator *authenticator) {
    qDebug() << Q_FUNC_INFO << fsub->alias();
    bool failed = false;
    QString gname = QString().number(fsub->forumId());
    settings->beginGroup("authentication");

    // Must exist and be longer than zero
    if(settings->contains(QString("%1/username").arg(gname)) &&
            settings->value(QString("%1/username").arg(gname)).toString().length() > 0) {
        QString key = QString("%1/failed").arg(fsub->forumId());
        if(settings->value(key).toString() == "true") failed = true;
        if(authenticator && !failed) {
            qDebug() << Q_FUNC_INFO << "reading u/p from settings";
            authenticator->setUser(settings->value(QString("%1/username").arg(gname)).toString());
            authenticator->setPassword(settings->value(QString("%1/password").arg(gname)).toString());
        }
    }
    settings->endGroup();
    if(!authenticator || authenticator->user().isNull() || failed) { // Ask user the credentials
        qDebug() << Q_FUNC_INFO << "asking user for http credentials";
        CredentialsRequest *cr = new CredentialsRequest(this);
        cr->subscription = fsub;
        cr->credentialType = CredentialsRequest::SH_CREDENTIAL_HTTP;
        credentialsRequests.append(cr);
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
            settings->beginGroup("authentication");
            settings->beginGroup(QString::number(currentCredentialsRequest->subscription->forumId()));
            settings->setValue("username", currentCredentialsRequest->authenticator.user());
            settings->setValue("password", currentCredentialsRequest->authenticator.password());
            settings->setValue("failed", "false");
            settings->endGroup();
            settings->endGroup();
            settings->sync();
        } else if(cr->credentialType == CredentialsRequest::SH_CREDENTIAL_FORUM) {
            qDebug() << Q_FUNC_INFO << "storing into subscription";
            currentCredentialsRequest->subscription->setUsername(currentCredentialsRequest->authenticator.user());
            currentCredentialsRequest->subscription->setPassword(currentCredentialsRequest->authenticator.password());
        }
    }
    // @todo move to SiilihaiSettings!!
    if(!currentCredentialsRequest->subscription->isAuthenticated()) {
        // Remove authentication
        settings->beginGroup("authentication");
        settings->remove(QString::number(currentCredentialsRequest->subscription->forumId()));
        settings->endGroup();

        currentCredentialsRequest->subscription->setUsername(QString::null);
        currentCredentialsRequest->subscription->setPassword(QString::null);
    }

    if(!noAccount())
        protocol.subscribeForum(currentCredentialsRequest->subscription); // Resend the forum infos

    UpdateEngine *engine = engines.value(currentCredentialsRequest->subscription);
    engine->credentialsEntered(currentCredentialsRequest);
    currentCredentialsRequest->deleteLater();
    currentCredentialsRequest=0;
    showNextCredentialsDialog();
}

void ClientLogic::syncProgress(float progress, QString message) {
    showStatusMessage(message);
}
