#include "forumprobe.h"
#include <QTextCodec>
#ifndef NO_WEBKITWIDGETS
#include <QWebFrame>
#include <QWebPage>
#endif
#include "tapatalk/tapatalkengine.h"
#include "discourse/discourseengine.h"
#include "messageformatting.h"

ForumProbe::ForumProbe(QObject *parent, SiilihaiProtocol *proto) :
    QObject(parent), m_protocol(proto), currentEngine(0), probedSub(0) {
    QObject::connect(&nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishedSlot(QNetworkReply*)));
}

void ForumProbe::probeUrl(QUrl urlToProbe, bool noServer) {
    qDebug() << Q_FUNC_INFO << urlToProbe.toString();
    Q_ASSERT(urlToProbe.isValid());
    Q_ASSERT(!currentEngine);
    typesProbed = 0;
    url = urlToProbe;
    if(probedSub) {
        probedSub->deleteLater();
        probedSub = 0;
    }
    if(noServer) {
        forumGot(0); // Skip server
    } else {
        connect(m_protocol, SIGNAL(forumGot(ForumSubscription*)), this, SLOT(forumGot(ForumSubscription*)));
        m_protocol->getForum(url);
    }
}

void ForumProbe::probeUrl(int id, bool noServer) {
    qDebug() << Q_FUNC_INFO << id;
    typesProbed = 0;

    Q_ASSERT(id > 0);
    if(probedSub) {
        probedSub->deleteLater();
        probedSub = 0;
    }
    if(noServer) {
        forumGot(0); // Skip server
    } else {
        connect(m_protocol, SIGNAL(forumGot(ForumSubscription*)), this, SLOT(forumGot(ForumSubscription*)));
        m_protocol->getForum(id);
    }
}

void ForumProbe::forumGot(ForumSubscription *sub) {
    qDebug() << Q_FUNC_INFO << url.toString() << sub;
    disconnect(m_protocol, SIGNAL(forumGot(ForumSubscription*)), this, SLOT(forumGot(ForumSubscription*)));
    if(!sub) { // Unknown forum - try to probe for known type
        probeNextType();
    } else { // Forum found from server - just use it
        qDebug() << Q_FUNC_INFO << "known forum " << sub->toString();
        emit probeResults(sub);
    }
}

void ForumProbe::engineProbeResults(ForumSubscription *sub) {
    qDebug() << Q_FUNC_INFO << url.toString() << sub;
    Q_ASSERT(!probedSub);
    typesProbed++;
    if(sub) {
        probedSub = ForumSubscription::newForProvider(sub->provider(), 0, true);
        probedSub->copyFrom(sub);
        if(!probedSub->forumUrl().isValid())
            probedSub->setForumUrl(url);

        if(sub->alias().isNull()) {
            qDebug() << Q_FUNC_INFO << "Getting title";
            // Dang, we need to figure out a name for the forum
            nam.get(QNetworkRequest(url));
        } else {
            emit probeResults(sub);
        }
    } else {
        if(typesProbed < FORUM_TYPE_COUNT) {
            probeNextType();
        } else {
            emit probeResults(0); // Nope, can't find anything valid
        }
    }
    currentEngine->deleteLater();
    currentEngine = 0;
}

QString ForumProbe::getTitle(QString &html) {
    QString title;
#ifndef NO_WEBKITWIDGETS
    QWebPage *page = new QWebPage();
    QWebFrame *frame = page->mainFrame();
    frame->setHtml(html);
    title = frame->title();
    page->deleteLater();
#else
    int titleBegin = html.indexOf("<title>");
    if(titleBegin > 0) {
        int titleEnd = html.indexOf("</", titleBegin);
        title = html.mid(titleBegin + 7, titleEnd - titleBegin - 7);
    }
#endif
    return title;
}

void ForumProbe::probeNextType()
{
    if(typesProbed == 0) {
        // Test for TapaTalk
        qDebug() << Q_FUNC_INFO << "unknown, checking for tapatalk";
        TapaTalkEngine *tte = new TapaTalkEngine(0, 0); // Deleted in engineProbeResults
        currentEngine = tte;
    } else if(typesProbed == 1) {
        qDebug() << Q_FUNC_INFO << "unknown, checking for discourse";
        DiscourseEngine *de = new DiscourseEngine(0,0);
        currentEngine = de;
    } else {
        Q_ASSERT(false); // You shouldn't call this anymore!!
    }
    connect(currentEngine, SIGNAL(urlProbeResults(ForumSubscription*)), this, SLOT(engineProbeResults(ForumSubscription*)));
    currentEngine->probeUrl(url);
}

// Title reply
void ForumProbe::finishedSlot(QNetworkReply *reply) {
    Q_ASSERT(probedSub);
    // no error received?
    if (reply->error() == QNetworkReply::NoError) {
        // Try to get page title
        QByteArray ba = reply->readAll();
        QTextCodec *codec = QTextCodec::codecForHtml(ba);
        QString html = codec->toUnicode(ba);
        QString title = getTitle(html);
        if(title.length() < 3) {
            qDebug() << Q_FUNC_INFO << "couldn't get title - setting url instead";
            title = reply->url().host();
        }
        qDebug() << Q_FUNC_INFO << "title:" << title << " codec: " << codec->name();
        probedSub->setAlias(MessageFormatting::stripHtml(title));
        Q_ASSERT(probedSub->provider() != ForumSubscription::FP_NONE);
        emit probeResults(probedSub);
    } else {
        emit probeResults(0);
    }
    reply->deleteLater();
}
