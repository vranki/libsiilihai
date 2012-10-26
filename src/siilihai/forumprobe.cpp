#include "forumprobe.h"
#include "tapatalk/tapatalkengine.h"

ForumProbe::ForumProbe(QObject *parent, SiilihaiProtocol &proto) :
    QObject(parent), protocol(proto), currentEngine(0), probedSub(0, true, ForumSubscription::FP_NONE) {
    QObject::connect(&nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishedSlot(QNetworkReply*)));
}

void ForumProbe::probeUrl(QUrl urlToProbe) {
    qDebug() << Q_FUNC_INFO << urlToProbe.toString();
    Q_ASSERT(urlToProbe.isValid());
    Q_ASSERT(!currentEngine);
    url = urlToProbe;
    probedSub.setProvider(ForumSubscription::FP_NONE);
    probedSub.setAlias(QString::null);
    probedSub.setForumId(0);
    probedSub.setForumUrl(url);
    connect(&protocol, SIGNAL(forumGot(ForumSubscription*)), this, SLOT(forumGot(ForumSubscription*)));
    protocol.getForum(url);
}

void ForumProbe::forumGot(ForumSubscription *sub) {
    qDebug() << Q_FUNC_INFO << url.toString() << sub;
    disconnect(&protocol, SIGNAL(forumGot(ForumSubscription*)), this, SLOT(forumGot(ForumSubscription*)));
    if(!sub) { // Unknown forum - get title Need to add it to DB!
        // Test for TapaTalk
        TapaTalkEngine *tte = new TapaTalkEngine(0, 0); // Deleted in engineProbeResults
        connect(tte, SIGNAL(urlProbeResults(ForumSubscription*)), this, SLOT(engineProbeResults(ForumSubscription*)));
        tte->probeUrl(url);
        currentEngine = tte;
    } else { // Forum found from server - just use it
        emit probeResults(sub);
    }
}

void ForumProbe::engineProbeResults(ForumSubscription *sub) {
    qDebug() << Q_FUNC_INFO << url.toString() << sub;

    if(sub)
        probedSub.setProvider(ForumSubscription::FP_TAPATALK);

    if(sub && sub->alias().isNull()) {
        // Dang, we need to figure out a name for the forum
        nam.get(QNetworkRequest(url));
    } else {
        emit probeResults(sub);
    }
    currentEngine->deleteLater();
    currentEngine = 0;
}

void ForumProbe::finishedSlot(QNetworkReply *reply) {
    qDebug() << Q_FUNC_INFO;
    // no error received?
    if (reply->error() == QNetworkReply::NoError) {
        QString html = reply->readAll();
        int titleBegin = html.indexOf("<title>");
        if(titleBegin > 0) {
            int titleEnd = html.indexOf("</", titleBegin);
            QString title = html.mid(titleBegin + 7, titleEnd - titleBegin - 7);
            qDebug() << Q_FUNC_INFO << "title:" << title;
            probedSub.setAlias(title);
            Q_ASSERT(probedSub.provider() != ForumSubscription::FP_NONE);
        }
        emit probeResults(&probedSub);
    } else {
        emit probeResults(0);
    }
    reply->deleteLater();
}

