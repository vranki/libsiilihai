#include "siilihaitool.h"
#include "siilihai/forumdata/forumsubscription.h"
#include "siilihai/forumdata/forumgroup.h"
#include "siilihai/forumdata/forumthread.h"
#include "siilihai/forumdata/forummessage.h"
#include "siilihai/updateengine.h"

SiilihaiTool::SiilihaiTool(QObject *parent) :
    QObject(parent), forumProbe(0, &protocol),
    m_currentSubscription(0), updateEngine(0), m_noServer(false), m_groupListReceived(false),
    m_threadListReceived(false), m_messageListReceived(false), m_groupBeingUpdated(0),
    m_threadBeingUpdated(0)
{
    providers << "[NONE  ]" << "[PARSER]" << "[TAPATA]" << "[DISCOU]" << "[ERROR ]";
}

SiilihaiTool::~SiilihaiTool()
{
}

void SiilihaiTool::setNoServer(bool ns)
{
    m_noServer = ns;
}

void SiilihaiTool::listForums()
{
    qDebug() << "Listing forums..";
    connect(&protocol, SIGNAL(listForumsFinished(QList<ForumSubscription*>)), this, SLOT(listForumsFinished(QList<ForumSubscription*>)));
    protocol.listForums();
}

void SiilihaiTool::getForum(int id)
{
    qDebug() << "Getting forum" << id << "...";
    protocol.getForum(id);
    connect(&protocol, SIGNAL(forumGot(ForumSubscription*)), this, SLOT(forumGot(ForumSubscription*)));
}

void SiilihaiTool::listForumsFinished(QList<ForumSubscription *> forums)
{
    qDebug() << "Received " << forums.size() << " forums:";
    foreach(auto fs, forums) {
        printForum(fs);
    }
}

void SiilihaiTool::forumGot(ForumSubscription *sub)
{
    qDebug() << "Received forum:";
    printForum(sub);
    m_currentSubscription = sub;
    if(command.isEmpty()) {
        QCoreApplication::quit();
    }
}

void SiilihaiTool::probe(QUrl url)
{
    qDebug() << "Probing " << url << "...";
    connect(&forumProbe, SIGNAL(probeResults(ForumSubscription*)), this, SLOT(probeResults(ForumSubscription*)));
    forumProbe.probeUrl(url, m_noServer);
}

void SiilihaiTool::listGroups(QUrl url)
{
    command = "list-groups";
    probe(url);
}

void SiilihaiTool::listThreads(QUrl url, QString groupId)
{
    command = "list-threads";
    m_groupId = groupId;
    probe(url);
}

void SiilihaiTool::listMessages(QUrl url, QString groupId, QString threadId)
{
    command = "list-messages";
    m_groupId = groupId;
    m_threadId = threadId;
    probe(url);
}


void SiilihaiTool::probeResults(ForumSubscription *probedSub)
{
    qDebug() << "Probe results: ";
    printForum(probedSub);
    m_currentSubscription = probedSub;
    if(command.isEmpty()) {
        QCoreApplication::quit();
    } else {
        updateEngine = UpdateEngine::newForSubscription(probedSub, 0, 0);
        connect(updateEngine, SIGNAL(groupListChanged(ForumSubscription*)), this, SLOT(groupListChanged(ForumSubscription*)));
        connect(updateEngine, SIGNAL(stateChanged(UpdateEngine*, UpdateEngine::UpdateEngineState, UpdateEngine::UpdateEngineState)),
                this, SLOT(engineStateChanged(UpdateEngine*, UpdateEngine::UpdateEngineState, UpdateEngine::UpdateEngineState)));
        updateEngine->setSubscription(probedSub);
    }
}

void SiilihaiTool::groupListChanged(ForumSubscription *sub)
{
    qDebug() << "Group list:";
    for(ForumGroup *group : sub->values()) {
        qDebug() << group->id() << ": " << group->name();
    }
    m_groupListReceived = true;
}

void SiilihaiTool::threadsChanged()
{
    qDebug() << "Thread list:";
    Q_ASSERT(m_groupBeingUpdated);
    for(ForumThread *thread : m_groupBeingUpdated->values()) {
        qDebug() << thread->id() << ": " << thread->name();
    }
    m_threadListReceived = true;
    if(command == "list-threads") QCoreApplication::quit();
}

void SiilihaiTool::messagesChanged()
{
    qDebug() << "Message list:";
    Q_ASSERT(m_threadBeingUpdated);
    for(ForumMessage *message : m_threadBeingUpdated->values()) {
        qDebug() << message->id() << ": " << message->name() << "by" << message->author();
    }
    m_messageListReceived = true;
}

void SiilihaiTool::engineStateChanged(UpdateEngine *engine, UpdateEngine::UpdateEngineState newState, UpdateEngine::UpdateEngineState oldState)
{
    Q_UNUSED(oldState);
    Q_UNUSED(engine);
    // qDebug() << "UE State:" << engine->stateName(newState);

    if(newState == UpdateEngine::UES_IDLE) {
        Q_ASSERT(m_currentSubscription);

        if(command == "list-groups") {
            if(!m_groupListReceived) {
                updateEngine->updateGroupList();
            } else {
                QCoreApplication::quit();
            }
        } else if(command == "list-threads") {
            if(!m_threadListReceived) {
                m_groupBeingUpdated = new ForumGroup(m_currentSubscription);
                m_groupBeingUpdated->setId(m_groupId);
                m_groupBeingUpdated->setSubscription(m_currentSubscription);
                m_groupBeingUpdated->setSubscribed(true);
                connect(m_groupBeingUpdated, SIGNAL(threadsChanged()), this, SLOT(threadsChanged()));
                updateEngine->updateGroup(m_groupBeingUpdated, true);
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
                updateEngine->updateThread(m_threadBeingUpdated, true);
            } else {
                QCoreApplication::quit();
            }
        }
    }
}

void SiilihaiTool::printForum(ForumSubscription *fs)
{
    if(fs) {
        qDebug() << fs->id() << providers[fs->provider()] << fs->alias() << fs->forumUrl().toString();
    } else {
        qDebug() << "  (no forum)";
    }
}
