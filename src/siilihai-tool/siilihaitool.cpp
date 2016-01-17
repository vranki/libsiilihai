#include "siilihaitool.h"
#include "siilihai/forumdata/forumsubscription.h"
#include "siilihai/forumdata/forumgroup.h"
#include "siilihai/updateengine.h"

SiilihaiTool::SiilihaiTool(QObject *parent) : QObject(parent), forumProbe(0, &protocol), currentSubscription(0), updateEngine(0)
{
    providers << "[NONE  ]" << "[PARSER]" << "[TAPATA]" << "[ERROR ]";
}

SiilihaiTool::~SiilihaiTool()
{
    if(currentSubscription) delete currentSubscription;
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
    currentSubscription = sub;
    if(command.isEmpty()) {
        QCoreApplication::quit();
    }
}

void SiilihaiTool::probe(QUrl url)
{
    qDebug() << "Probing " << url << "...";
    connect(&forumProbe, SIGNAL(probeResults(ForumSubscription*)), this, SLOT(probeResults(ForumSubscription*)));
    forumProbe.probeUrl(url);
}

void SiilihaiTool::listGroups(QUrl url)
{
    command = "list-groups";
    probe(url);
}


void SiilihaiTool::probeResults(ForumSubscription *probedSub)
{
    qDebug() << "Probe results: ";
    printForum(probedSub);
    currentSubscription = probedSub;
    if(command.isEmpty()) {
        QCoreApplication::quit();
    } else if(command == "list-groups") {
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
        qDebug() << group->name();
    }
}

void SiilihaiTool::engineStateChanged(UpdateEngine *engine, UpdateEngine::UpdateEngineState newState, UpdateEngine::UpdateEngineState oldState)
{
    qDebug() << "UE State:" << engine->stateName(newState);
    if(newState == UpdateEngine::UES_IDLE) {
        if(command == "list-groups") {
            updateEngine->updateGroupList();
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
