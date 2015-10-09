#include "siilihaitool.h"
#include "siilihai/forumdata/forumsubscription.h"

SiilihaiTool::SiilihaiTool(QObject *parent) : QObject(parent), forumProbe(0, protocol)
{
    providers << "[NONE  ]" << "[PARSER]" << "[TAPATA]" << "[ERROR ]";
}

SiilihaiTool::~SiilihaiTool()
{
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
    QCoreApplication::quit();
}

void SiilihaiTool::forumGot(ForumSubscription *sub)
{
    qDebug() << "Received forum:";
    printForum(sub);
    sub->deleteLater();
    QCoreApplication::quit();
}

void SiilihaiTool::probe(QUrl url)
{
    qDebug() << "Probing " << url << "...";
    connect(&forumProbe, SIGNAL(probeResults(ForumSubscription*)), this, SLOT(probeResults(ForumSubscription*)));
    forumProbe.probeUrl(url);
}


void SiilihaiTool::probeResults(ForumSubscription *probedSub)
{
    qDebug() << "Probe results: ";
    printForum(probedSub);
    probedSub->deleteLater();
    QCoreApplication::quit();
}

void SiilihaiTool::printForum(ForumSubscription *fs)
{
    if(fs) {
        qDebug() << fs->id() << providers[fs->provider()] << fs->alias() << fs->forumUrl().toString();
    } else {
        qDebug() << "  (no forum)";
    }
}

