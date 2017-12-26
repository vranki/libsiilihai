#include "siilihaitool.h"
#include "siilihai/forumdata/forumsubscription.h"
#include "siilihai/forumdata/forumgroup.h"
#include "siilihai/forumdata/forumthread.h"
#include "siilihai/forumdata/forummessage.h"
#include "siilihai/updateengine.h"
#include "siilihai/parser/parserengine.h"

SiilihaiTool::SiilihaiTool(QObject *parent) :
    QObject(parent)
  , forumProbe(nullptr, &protocol)
  , parserManager(this, &protocol)
  , m_currentSubscription(nullptr)
  , updateEngine(nullptr)
  , m_noServer(false)
  , m_groupListReceived(false)
  , m_threadListReceived(false)
  , m_messageListReceived(false)
  , m_groupBeingUpdated(nullptr)
  , m_threadBeingUpdated(nullptr)
  , m_forumId(0)
  , m_success(false)
{
    providers << "[NONE  ]" << "[PARSER]" << "[TAPATA]" << "[DISCOU]" << "[ERROR ]";
    protocol.setOffline(false);
    connect(&parserManager, &ParserManager::parserUpdated, this, &SiilihaiTool::parserUpdated);
}

SiilihaiTool::~SiilihaiTool() {}

void SiilihaiTool::setNoServer(bool ns) {
    m_noServer = ns;
}

void SiilihaiTool::setForumId(const int id) {
    if(!m_currentSubscription || m_currentSubscription->id() != id) {
        m_forumId = id;
    }
}

void SiilihaiTool::setForumUrl(const QUrl url)
{
    if(!m_currentSubscription || m_currentSubscription->forumUrl() != url) {
        m_forumUrl = url;
    }
}

bool SiilihaiTool::success()
{
    return m_success;
}

void SiilihaiTool::listForums()
{
    if(m_noServer) {
        qWarning() << "Can't list forums without using server!";
        QCoreApplication::quit();
    }
    qInfo() << "Listing forums..";
    connect(&protocol, SIGNAL(listForumsFinished(QList<ForumSubscription*>)), this, SLOT(listForumsFinished(QList<ForumSubscription*>)));
    protocol.listForums();
}

void SiilihaiTool::getForum(const int id)
{
    qInfo() << "Getting forum" << id << "...";
    connect(&protocol, SIGNAL(forumGot(ForumSubscription*)), this, SLOT(forumGot(ForumSubscription*)));
    protocol.getForum(id);
}

void SiilihaiTool::listForumsFinished(QList<ForumSubscription *> forums) {
    qInfo() << "Received " << forums.size() << " forums:";
    for(auto fs : forums) {
        printForum(fs);
    }
    QCoreApplication::quit();
}

void SiilihaiTool::forumGot(ForumSubscription *tempSub) {
    if(tempSub) {
        qInfo() << "Received forum from server:";
        printForum(tempSub);
        copyToCurrentSubscription(tempSub);
        performCommand();
    } else {
        qWarning() << "Unable to get forum.";
        QCoreApplication::quit();
    }
}

void SiilihaiTool::probe() {
    if(forumProbe.isProbing()) {
        qInfo() << "Already probing..";
        return;
    }
    if(m_forumId && !m_noServer) {
        qInfo() << "Getting info for forum " << m_forumId << "...";
        getForum(m_forumId);
    } else if(m_forumUrl.isValid()) {
        qInfo() << "Probing " << m_forumUrl.toString() << (m_noServer ? "without asking server" : "using server") << "...";
        connect(&forumProbe, SIGNAL(probeResults(ForumSubscription*)), this, SLOT(probeResults(ForumSubscription*)));
        forumProbe.probeUrl(m_forumUrl, m_noServer);
    } else {
        qWarning() << "Please set forum ID or URL to probe";
        QCoreApplication::quit();
    }
}

void SiilihaiTool::listGroups() {
    command = "list-groups";
    probe();
}

void SiilihaiTool::listThreads(QString groupId) {
    command = "list-threads";
    m_groupId = groupId;
    probe();
}

void SiilihaiTool::listMessages(QString groupId, QString threadId) {
    command = "list-messages";
    m_groupId = groupId;
    m_threadId = threadId;
    probe();
}

void SiilihaiTool::updateForum()
{
    command = "update-forum";
    probe();
}

void SiilihaiTool::probeResults(ForumSubscription *probedSub) {
    qInfo() << "Probe results: ";
    printForum(probedSub);
    copyToCurrentSubscription(probedSub);
    performCommand();
}

void SiilihaiTool::groupListChanged(ForumSubscription *sub) {
    qInfo() << "Group list:";
    for(ForumGroup *group : sub->values()) {
        qInfo() << group->id() << ": " << group->name() << group->lastchange();
    }
    m_groupListReceived = true;
    if(command == "list-groups") {
        m_success = !sub->isEmpty();
        QCoreApplication::quit();
    }
}

void SiilihaiTool::threadsChanged() {
    qInfo() << "Thread list:";
    Q_ASSERT(m_groupBeingUpdated);
    for(ForumThread *thread : m_groupBeingUpdated->values()) {
        qInfo() << thread->id() << ": " << thread->name() << thread->lastchange();
    }
    qInfo() << "\nTotal" << m_groupBeingUpdated->size() << "threads.";
    if(m_groupBeingUpdated->size() == m_currentSubscription->latestThreads()) {
        qInfo() << "Update limit of " << m_currentSubscription->latestThreads() << " threads reached.";
    }
    m_threadListReceived = true;
    if(command == "list-threads") {
        m_success = !m_groupBeingUpdated->isEmpty();
        QCoreApplication::quit();
    }
}

void SiilihaiTool::messagesChanged() {
    qInfo() << "Message list:";
    Q_ASSERT(m_threadBeingUpdated);
    for(ForumMessage *message : m_threadBeingUpdated->values()) {
        qDebug() << message->id() << ": " << message->name() << "by" << message->author();
    }
    qInfo() << "\nTotal" << m_threadBeingUpdated->size() << "messages.";
    if(m_threadBeingUpdated->size() == m_currentSubscription->latestMessages()) {
        qInfo() << "Update limit of " << m_currentSubscription->latestMessages() << " messages reached.";
    }
    m_messageListReceived = true;
    if(command == "list-messages") {
        m_success = !m_threadBeingUpdated->isEmpty();
        QCoreApplication::quit();
    }
}

void SiilihaiTool::engineStateChanged(UpdateEngine *engine, UpdateEngine::UpdateEngineState newState, UpdateEngine::UpdateEngineState oldState)
{
    qInfo() << "Update engine state:" << engine->stateName(newState);
    if(newState == UpdateEngine::UES_ERROR) {
        printForumErrors(updateEngine->subscription());
        QCoreApplication::quit();
    }

    if(newState == UpdateEngine::UES_IDLE && oldState == UpdateEngine::UES_UPDATING) {
        QCoreApplication::quit();
    } else if(newState == UpdateEngine::UES_IDLE && oldState != UpdateEngine::UES_UPDATING) {
        Q_ASSERT(m_currentSubscription);

        if(command == "list-groups") {
            updateEngine->updateGroupList();
        } else if(command == "list-threads") {
            if(!m_threadListReceived) {
                m_groupBeingUpdated = new ForumGroup(m_currentSubscription);
                m_groupBeingUpdated->setId(m_groupId);
                m_groupBeingUpdated->setSubscription(m_currentSubscription);
                m_groupBeingUpdated->setSubscribed(true);
                connect(m_groupBeingUpdated, SIGNAL(threadsChanged()), this, SLOT(threadsChanged()));
                updateEngine->updateGroup(m_groupBeingUpdated, true, true);
            } else {
                QCoreApplication::quit();
            }
        } else if(command == "list-messages") {
            if(!m_messageListReceived) {
                m_groupBeingUpdated = new ForumGroup(m_currentSubscription);
                m_groupBeingUpdated->setId(m_groupId);
                m_groupBeingUpdated->setSubscription(m_currentSubscription);
                m_groupBeingUpdated->setSubscribed(true);

                m_threadBeingUpdated = new ForumThread(m_groupBeingUpdated);
                m_threadBeingUpdated->setGroup(m_groupBeingUpdated);
                m_threadBeingUpdated->setId(m_threadId);
                connect(m_threadBeingUpdated, SIGNAL(messagesChanged()), this, SLOT(messagesChanged()));
                updateEngine->updateThread(m_threadBeingUpdated, true, true);
            } else {
                QCoreApplication::quit();
            }
        } else if(command == "update-forum") {
            connect(updateEngine, &UpdateEngine::progressReport, this, &SiilihaiTool::progressReport);
            updateEngine->updateForum();
        }
    }
}

void SiilihaiTool::parserUpdated(ForumParser *newParser)
{
    if(newParser) {
        qInfo() << "Downloaded parser for " << newParser->name() << "...";
    } else {
        qWarning() << "Didn't get parser for " << m_currentSubscription->id();
        QCoreApplication::quit();
    }
}

void SiilihaiTool::progressReport(ForumSubscription *forum, float progress)
{
    qInfo() << forum->alias() << progress * 100 << "% updated";
}

void SiilihaiTool::printForum(ForumSubscription *fs) {
    if(fs) {
        qInfo() << qSetFieldWidth(3)
                << fs->id()
                << qSetFieldWidth(5) << providers[fs->provider()]
                << qSetFieldWidth(0) << fs->alias() << fs->forumUrl().toString();
    } else {
        qInfo() << "  (no forum)";
    }
}

void SiilihaiTool::printForumErrors(ForumSubscription *sub)
{
    if(!sub->errorList().isEmpty()) {
        qWarning() << "Update errors were reported:";
        for(auto error : updateEngine->subscription()->errorList()) {
            qWarning() << error->title() << error->description() << error->technicalData();
        }
        updateEngine->subscription()->clearErrors();
    }
}

void SiilihaiTool::copyToCurrentSubscription(const ForumSubscription *tempSub)
{
    // Copy temp sub to a local one
    ForumSubscription *newSub = ForumSubscription::newForProvider(tempSub->provider(), this, false);
    newSub->copyFrom(tempSub);
    m_currentSubscription = newSub;
}

void SiilihaiTool::performCommand()
{
    if(command.isEmpty()) {
        QCoreApplication::quit();
    } else {
        Q_ASSERT(!updateEngine);
        updateEngine = UpdateEngine::newForSubscription(m_currentSubscription, nullptr, &parserManager);
        connect(updateEngine, SIGNAL(groupListChanged(ForumSubscription*)), this, SLOT(groupListChanged(ForumSubscription*)));
        connect(updateEngine, SIGNAL(stateChanged(UpdateEngine*, UpdateEngine::UpdateEngineState, UpdateEngine::UpdateEngineState)),
                this, SLOT(engineStateChanged(UpdateEngine*, UpdateEngine::UpdateEngineState, UpdateEngine::UpdateEngineState)));
        updateEngine->setSubscription(m_currentSubscription);
    }
}
