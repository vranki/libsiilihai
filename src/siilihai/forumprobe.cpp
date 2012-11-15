#include "forumprobe.h"
#include "tapatalk/tapatalkengine.h"

ForumProbe::ForumProbe(QObject *parent, SiilihaiProtocol &proto) :
    QObject(parent), protocol(proto), currentEngine(0), probedSub(0) {
    QObject::connect(&nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishedSlot(QNetworkReply*)));
}

void ForumProbe::probeUrl(QUrl urlToProbe) {
    qDebug() << Q_FUNC_INFO << urlToProbe.toString();
    Q_ASSERT(urlToProbe.isValid());
    Q_ASSERT(!currentEngine);
    url = urlToProbe;
    if(probedSub) {
        probedSub->deleteLater();
        probedSub = 0;
    }
    connect(&protocol, SIGNAL(forumGot(ForumSubscription*)), this, SLOT(forumGot(ForumSubscription*)));
    protocol.getForum(url);
}

void ForumProbe::probeUrl(int id) {
    qDebug() << Q_FUNC_INFO << id;
    Q_ASSERT(id > 0);
    if(probedSub) {
        probedSub->deleteLater();
        probedSub = 0;
    }
    connect(&protocol, SIGNAL(forumGot(ForumSubscription*)), this, SLOT(forumGot(ForumSubscription*)));
    protocol.getForum(id);
}

void ForumProbe::forumGot(ForumSubscription *sub) {
    qDebug() << Q_FUNC_INFO << url.toString() << sub;
    disconnect(&protocol, SIGNAL(forumGot(ForumSubscription*)), this, SLOT(forumGot(ForumSubscription*)));
    if(!sub) { // Unknown forum - get title Need to add it to DB!
        // Test for TapaTalk
        qDebug() << Q_FUNC_INFO << "unknown, checking for tapatalk";
        TapaTalkEngine *tte = new TapaTalkEngine(0, 0); // Deleted in engineProbeResults
        connect(tte, SIGNAL(urlProbeResults(ForumSubscription*)), this, SLOT(engineProbeResults(ForumSubscription*)));
        tte->probeUrl(url);
        currentEngine = tte;
    } else { // Forum found from server - just use it
        qDebug() << Q_FUNC_INFO << "known forum " << sub->toString();
        emit probeResults(sub);
    }
}

void ForumProbe::engineProbeResults(ForumSubscription *sub) {
    qDebug() << Q_FUNC_INFO << url.toString() << sub;
    Q_ASSERT(!probedSub);
    if(sub) {
        probedSub = ForumSubscription::newForProvider(sub->provider(), 0, true);
        probedSub->copyFrom(sub);
        if(!probedSub->forumUrl().isValid())
            probedSub->setForumUrl(url);
        qDebug() << Q_FUNC_INFO << sub->toString();
        if(sub->alias().isNull()) {
            qDebug() << Q_FUNC_INFO << "Getting title";
            // Dang, we need to figure out a name for the forum
            nam.get(QNetworkRequest(url));
        } else {
            emit probeResults(sub);
        }
    } else {
        emit probeResults(sub);
    }
    currentEngine->deleteLater();
    currentEngine = 0;
}

// Title reply
void ForumProbe::finishedSlot(QNetworkReply *reply) {
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT(probedSub);
    // no error received?
    if (reply->error() == QNetworkReply::NoError) {
        QString html = reply->readAll();
        int titleBegin = html.indexOf("<title>");
        if(titleBegin > 0) {
            int titleEnd = html.indexOf("</", titleBegin);
            QString title = html.mid(titleBegin + 7, titleEnd - titleBegin - 7);
            qDebug() << Q_FUNC_INFO << "title:" << title;
            probedSub->setAlias(title);
            Q_ASSERT(probedSub->provider() != ForumSubscription::FP_NONE);
        }
        emit probeResults(probedSub);
    } else {
        emit probeResults(0);
    }
    reply->deleteLater();
}
