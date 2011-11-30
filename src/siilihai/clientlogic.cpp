#include "clientlogic.h"
#include <time.h>
#include <QDir>
#include <QNetworkProxy>
#include "parsermanager.h"
#include "forumsubscription.h"
#include "forumgroup.h"
#include "forumthread.h"

ClientLogic::ClientLogic(QObject *parent) : QObject(parent), forumDatabase(this), syncmaster(this, forumDatabase, protocol),
    settings(0), parserManager(0)
{
    endSyncDone = false;
    firstRun = true;
    dbStored = false;
    srand ( time(NULL) );
    // Make sure Siilihai::subscriptionFound is called first to get ParserEngine
    connect(&forumDatabase, SIGNAL(subscriptionFound(ForumSubscription*)), this, SLOT(subscriptionFound(ForumSubscription*)));
    connect(&forumDatabase, SIGNAL(databaseStored()), this, SLOT(databaseStored()), Qt::QueuedConnection);
    connect(&protocol, SIGNAL(userSettingsReceived(bool,UserSettings*)), this, SLOT(userSettingsReceived(bool,UserSettings*)));
}

void ClientLogic::launchSiilihai() {
    changeState(SH_STARTED);
    settings = new QSettings(getDataFilePath() + "/siilihai_settings.ini", QSettings::IniFormat, this);

    firstRun = settings->value("first_run", true).toBool();
    settings->setValue("first_run", false);

    QString proxy = settings->value("preferences/http_proxy", "").toString();
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

    protocol.setBaseURL(settings->value("network/baseurl", BASEURL).toString());

    QString databaseFileName = getDataFilePath() + "/siilihai_forums.xml";

    if(firstRun) {
        forumDatabase.openDatabase(databaseFileName); // Fails
    } else {
        int currentSchemaVersion = settings->value("forum_database_schema", 0).toInt();
        if(forumDatabase.schemaVersion() != currentSchemaVersion) {
            errorDialog("The database schema has been changed. Your forum database will be reset. Sorry. ");
            forumDatabase.openDatabase(databaseFileName);
        } else {
            if(!forumDatabase.openDatabase(databaseFileName)) {
                errorDialog("Could not open Siilihai's forum database file.\n"
                            "See console for details.");
            }
        }
    }
    settings->setValue("forum_database_schema", forumDatabase.schemaVersion());
    settings->sync();

#ifdef Q_WS_HILDON
    if(firstRun) {
        errorDialog("This is beta software\nMaemo version can't connect\n"
                    "to the Internet, so please do it manually before continuing.\n"
                    "The UI is also work in progress. Go to www.siilihai.com for details.");
        settings->setValue("firstrun", false);
    }
#endif

    if (settings->value("account/username", "").toString().isEmpty()) {
        showLoginWizard();
    } else {
        showMainWindow();
        tryLogin();
    }
}

void ClientLogic::haltSiilihai() {
    cancelClicked();
    foreach(ParserEngine *engine, engines.values())
        engine->deleteLater();
    engines.clear();
    if(usettings.syncEnabled() && !endSyncDone && currentState == SH_READY) {
        qDebug() << "Sync enabled - running end sync";
        changeState(SH_ENDSYNC);
        syncmaster.endSync();
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
    return ".";
}

void ClientLogic::settingsChanged(bool byUser) {
    usettings.setSyncEnabled(settings->value("preferences/sync_enabled", false).toBool());
    if(byUser) {
        protocol.setUserSettings(&usettings);
    }
    settings->sync();
}

void ClientLogic::tryLogin() {
    Q_ASSERT(currentState==SH_STARTED || currentState==SH_OFFLINE);
    changeState(SH_LOGIN);

    connect(&protocol, SIGNAL(loginFinished(bool, QString,bool)), this, SLOT(loginFinished(bool, QString,bool)));
    protocol.login(settings->value("account/username", "").toString(),
                   settings->value("account/password", "").toString());
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
    } else if(newState==SH_STARTSYNCING) {
        qDebug() << Q_FUNC_INFO << "Startsync";
        if(usettings.syncEnabled())
            syncmaster.startSync();
    } else if(newState==SH_ENDSYNC) {
        qDebug() << Q_FUNC_INFO << "Endsync";
    } else if(newState==SH_STOREDB) {
        qDebug() << Q_FUNC_INFO << "Storedb";
        if(!forumDatabase.storeDatabase()) {
            errorDialog("Failed to save forum database file");
        }
        dbStored = true;
    } else if(newState==SH_READY) {
        qDebug() << Q_FUNC_INFO << "Ready";
        if(settings->value("preferences/update_automatically", true).toBool())
            updateClicked();

        if (forumDatabase.isEmpty()) { // Display subscribe dialog if none subscribed
            subscribeForum();
        }
    }
}

void ClientLogic::updateClicked() {
    if(currentState != SH_READY) return;
    int parsersUpdating = 0;
    foreach(ParserEngine* engine, engines.values()) {
        if(parsersUpdating <= MAX_CONCURRENT_UPDATES) {
            if(engine->state()==ParserEngine::PES_IDLE) {
                engine->updateForum();
                parsersUpdating++;
            }
        } else {
            subscriptionsToUpdateLeft.append(engine->subscription());
        }
    }
}

void ClientLogic::updateClicked(ForumSubscription* sub , bool force) {
    if(currentState != SH_READY) return;

    Q_ASSERT(engines.contains(sub));
    ParserEngine *engine = engines.value(sub);
    if(engine && engine->state()==ParserEngine::PES_IDLE && currentState != SH_OFFLINE && currentState != SH_STARTED)
        engine->updateForum(force);
}

void ClientLogic::cancelClicked() {
    foreach(ParserEngine* engine, engines.values())
        engine->cancelOperation();
}


void ClientLogic::syncFinished(bool success, QString message){
    qDebug() << Q_FUNC_INFO << success;
    if (!success) {
        errorDialog(QString("Syncing status to server failed.\n\n%1").arg(message));
    }
    if(currentState == SH_STARTSYNCING) {
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
        foreach(int serverSubscriptionId, serversSubscriptions) {
            if (serverSubscriptionId == sub->parser())
                found = true;
        }
        if (!found) {
            unsubscribedForums.append(sub);
        }
    }
    foreach (ForumSubscription *sub, unsubscribedForums) {
        parserManager->deleteParser(sub->parser());
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
        errorDialog("Error: Login failed. Check your username, password and network connection.\nWorking offline.");
        changeState(SH_OFFLINE);
    }
}

void ClientLogic::forumAdded(ForumSubscription *fs) {
    if(forumDatabase.contains(fs->parser())) {
        errorDialog("You have already subscribed to " + fs->alias());
    } else {
        ForumSubscription *newFs = new ForumSubscription(&forumDatabase, false);
        newFs->copyFrom(fs);
        fs = 0;
        Q_ASSERT(parserManager->getParser(newFs->parser())); // Should already be there!
        ParserEngine *newEngine = new ParserEngine(&forumDatabase, this, parserManager, nam);
        newEngine->setParser(parserManager->getParser(newFs->parser()));
        newEngine->setSubscription(newFs);
        Q_ASSERT(!engines.contains(newFs));
        engines[newFs] = newEngine;

        if(!forumDatabase.addSubscription(newFs)) { // Emits subscriptionFound
            errorDialog("Error: Unable to subscribe to forum. Check the log.");
        } else {
            newEngine->updateGroupList();
            protocol.subscribeForum(newFs);
        }
    }
}

void ClientLogic::subscriptionDeleted(QObject* subobj) {
    ForumSubscription *sub = static_cast<ForumSubscription*> (subobj);
    if(!engines.contains(sub)) return; // Possible when quitting
    engines[sub]->cancelOperation();
    engines[sub]->deleteLater();
    engines.remove(sub);
}

void ClientLogic::forumUpdated(ForumSubscription* forum) {
    int busyForums = 0;
    foreach(ForumSubscription *sub, forumDatabase.values()) {
        if(sub->parserEngine()->state()==ParserEngine::PES_UPDATING) {
            busyForums++;
        }
    }

    subscriptionsToUpdateLeft.removeAll(forum);
    ForumSubscription *nextSub = 0;
    while(!nextSub && !subscriptionsToUpdateLeft.isEmpty()
          && busyForums <= MAX_CONCURRENT_UPDATES) {
        nextSub = subscriptionsToUpdateLeft.takeFirst();
        if(forumDatabase.values().contains(nextSub)) {
            nextSub->parserEngine()->updateForum();
            busyForums++;
        }
    }
    if(!busyForums)
        forumDatabase.storeDatabase();
}

void ClientLogic::subscribeForumFinished(ForumSubscription *sub, bool success) {
    if (!success) {
        errorDialog("Subscribing to forum failed. Please check network connection.");
        if(forumDatabase.value(sub->parser()))
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

void ClientLogic::updateFailure(ForumSubscription* sub, QString msg) {
    settings->setValue(QString("authentication/%1/failed").arg(sub->parser()), "true");
    errorDialog(sub->alias() + "\n" + msg);
}

void ClientLogic::moreMessagesRequested(ForumThread* thread) {
    Q_ASSERT(thread);
    ParserEngine *engine = engines[thread->group()->subscription()];
    Q_ASSERT(engine);
    if(engine->state() == ParserEngine::PES_UPDATING) return;

    thread->setGetMessagesCount(thread->getMessagesCount() + settings->value("preferences/show_more_count", 30).toInt());
    thread->commitChanges();
    engine->updateThread(thread);
}

void ClientLogic::unsubscribeGroup(ForumGroup *group) {
    group->setSubscribed(false);
    group->commitChanges();
    protocol.subscribeGroups(group->subscription());
}

void ClientLogic::forumUpdateNeeded(ForumSubscription *fs) {
    qDebug() << Q_FUNC_INFO;
    protocol.subscribeForum(fs);
    updateClicked(fs);
}

void ClientLogic::unregisterSiilihai() {
    cancelClicked();
    forumDatabase.resetDatabase();
    settings->remove("account/username");
    settings->remove("account/password");
    settings->remove("first_run");
    forumDatabase.storeDatabase();
    usettings.setSyncEnabled(false);
    errorDialog("Siilihai has been unregistered and will now quit.");
    haltSiilihai();
}
void ClientLogic::databaseStored() {
    if(currentState==SH_STOREDB)
        haltSiilihai();
}

// Caution - engine->subscription() may be null (when deleted)!
void ClientLogic::parserEngineStateChanged(ParserEngine *engine, ParserEngine::ParserEngineState newState, ParserEngine::ParserEngineState oldState) {
    if(newState != ParserEngine::PES_REQUESTING_CREDENTIALS  && currentState==SH_READY && newState == ParserEngine::PES_IDLE && oldState != ParserEngine::PES_UPDATING) {
        if (settings->value("preferences/update_automatically", true).toBool())
            updateClicked(engine->subscription());
    }
}

void ClientLogic::loginWizardFinished() {
    if (settings->value("account/username", "").toString().length() == 0) {
        haltSiilihai();
    } else {
        showMainWindow();
        settingsChanged(false);
        loginFinished(true, QString(), usettings.syncEnabled());
    }
}

void ClientLogic::subscriptionFound(ForumSubscription *sub) {
    connect(sub, SIGNAL(destroyed(QObject*)), this, SLOT(subscriptionDeleted(QObject*)));
    ParserEngine *pe = engines.value(sub);
    if(!pe) {
        pe = new ParserEngine(&forumDatabase, this, parserManager, nam);
        pe->setSubscription(sub);
        engines[sub] = pe;
    }
    connect(pe, SIGNAL(groupListChanged(ForumSubscription*)), this, SLOT(showSubscribeGroup(ForumSubscription*)));
    connect(pe, SIGNAL(forumUpdated(ForumSubscription*)), this, SLOT(forumUpdated(ForumSubscription*)));
    connect(pe, SIGNAL(updateFailure(ForumSubscription*, QString)), this, SLOT(updateFailure(ForumSubscription*, QString)));
    connect(pe, SIGNAL(getAuthentication(ForumSubscription*, QAuthenticator*)), this, SLOT(getAuthentication(ForumSubscription*,QAuthenticator*)));
    connect(pe, SIGNAL(loginFinished(ForumSubscription*,bool)), this, SLOT(forumLoginFinished(ForumSubscription*,bool)));
    connect(pe, SIGNAL(stateChanged(ParserEngine *, ParserEngine::ParserEngineState, ParserEngine::ParserEngineState)),
            this, SLOT(parserEngineStateChanged(ParserEngine *, ParserEngine::ParserEngineState, ParserEngine::ParserEngineState)));
    connect(pe, SIGNAL(updateForumSubscription(ForumSubscription *)), &protocol, SLOT(subscribeForum(ForumSubscription *)));
    if(!pe->parser()) pe->setParser(parserManager->getParser(sub->parser())); // Load the (possibly old) parser
    Q_ASSERT(sub->parserEngine());
}

void ClientLogic::updateGroupSubscriptions(ForumSubscription *sub) {
    protocol.subscribeGroups(sub);
    engines.value(sub)->updateForum();
}

void ClientLogic::updateThread(ForumThread* thread, bool force) {
    if(currentState != SH_READY) return;
    ForumSubscription *sub = thread->group()->subscription();
    Q_ASSERT(sub);
    Q_ASSERT(engines.contains(sub));
    engines[sub]->updateThread(thread, force);
}

void ClientLogic::forumLoginFinished(ForumSubscription *sub, bool success) {
    if(!success)
        errorDialog(QString("Login to %1 failed. Please check credentials.").arg(sub->alias()));
}

void ClientLogic::unsubscribeForum(ForumSubscription* fs) {
    protocol.subscribeForum(fs, true);
    forumDatabase.deleteSubscription(fs);
    parserManager->deleteParser(fs->parser());
}

void ClientLogic::getAuthentication(ForumSubscription *fsub, QAuthenticator *authenticator) {
    bool failed = false;
    QString gname = QString().number(fsub->parser());
    settings->beginGroup("authentication");
    if(settings->contains(QString("%1/username").arg(gname))) {
        authenticator->setUser(settings->value(QString("%1/username").arg(gname)).toString());
        authenticator->setPassword(settings->value(QString("%1/password").arg(gname)).toString());
        if(settings->value(QString("authentication/%1/failed").arg(gname)).toString() == "true") failed = true;
    }
    settings->endGroup();
    if(authenticator->user().isNull() || failed) {
        showCredentialsDialog(fsub, authenticator);
    }
}
