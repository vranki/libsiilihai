#include "subscriptionmanagement.h"
#include "siilihaiprotocol.h"
#include "siilihaisettings.h"

SubscriptionManagement::SubscriptionManagement(QObject *parent, SiilihaiProtocol *protocol, SiilihaiSettings *settings)
    : QObject(parent)
    , m_protocol(protocol)
    , m_newForum(nullptr)
    , m_probe(nullptr, m_protocol)
    , m_settings(settings)
    , m_probeInProgress(false)
{
    Q_ASSERT(m_settings);
    Q_ASSERT(m_protocol);
    connect(m_protocol, SIGNAL(subscribeForumFinished(ForumSubscription*, bool)), this, SLOT(subscribeForumFinished(ForumSubscription*,bool)));
    connect(&m_probe, SIGNAL(probeResults(ForumSubscription*)), this, SLOT(probeResults(ForumSubscription*)));
}

SubscriptionManagement::~SubscriptionManagement()
{
    qDeleteAll(m_forumList);
    m_forumList.clear();
}

QList<QObject *> SubscriptionManagement::forumList()
{
    QList<QObject*> forumListForums;
    QString filterLc = m_forumFilter.toLower();
    for(ForumSubscription *sub : m_forumList) {
        QString aliasLc = sub->alias().toLower();
        QString urlLc = sub->forumUrl().toString().toLower();
        if (aliasLc.contains(m_forumFilter) || urlLc.contains(m_forumFilter)) {
            forumListForums.append(qobject_cast<QObject*>(sub));
        }
    }

    return forumListForums;
}

QString SubscriptionManagement::forumFilter() const
{
    return m_forumFilter;
}

ForumSubscription *SubscriptionManagement::newForum() const
{
    return m_newForum;
}

bool SubscriptionManagement::probeInProgress() const
{
    return m_probeInProgress;
}

void SubscriptionManagement::setForumFilter(QString forumFilter)
{
    if (m_forumFilter == forumFilter)
        return;

    m_forumFilter = forumFilter;
    emit forumFilterChanged(forumFilter);
    emit forumListChanged();
}

// Receiver owns the forums!
void SubscriptionManagement::listForumsFinished(QList <ForumSubscription*> forums) {
    disconnect(m_protocol, &SiilihaiProtocol::listForumsFinished, this, &SubscriptionManagement::listForumsFinished);
    qDeleteAll(m_forumList);
    m_forumList.clear();
    m_forumList.append(forums);
    emit forumListChanged();
}

void SubscriptionManagement::listForums()
{
    resetNewForum();
    // This signal can be emitted by other places, so listen to it only
    // when we're listing really.
    connect(m_protocol, &SiilihaiProtocol::listForumsFinished, this, &SubscriptionManagement::listForumsFinished);
    m_protocol->listForums();
}

void SubscriptionManagement::unsubscribeForum(ForumSubscription *fs)
{
    emit forumUnsubscribed(fs);
}

void SubscriptionManagement::getForum(int id)
{
    Q_ASSERT(!m_probeInProgress);
    m_probeInProgress = true;
    emit probeInProgressChanged(m_probeInProgress);

    m_probe.probeUrl(id);
}

void SubscriptionManagement::getForum(QUrl url)
{
    Q_ASSERT(!m_probeInProgress);
    m_probeInProgress = true;
    emit probeInProgressChanged(m_probeInProgress);

    m_probe.probeUrl(url);
}

void SubscriptionManagement::subscribeThisForum(QString user, QString pass)
{
    if(!m_newForum) return;

    m_newForum->setAuthenticated(!user.isEmpty());
    if(m_newForum->isAuthenticated()) {
        m_newForum->setUsername(user);
        m_newForum->setPassword(pass);
    }
    m_newForum->setLatestThreads(m_settings->threadsPerGroup());
    m_newForum->setLatestMessages(m_settings->messagesPerThread());

    emit forumAdded(m_newForum);
}

void SubscriptionManagement::resetNewForum()
{
    if(m_probeInProgress) {
        m_probeInProgress = false;
        emit probeInProgressChanged(m_probeInProgress);
    }
    if(m_newForum) {
        m_newForum->deleteLater();
        m_newForum = nullptr;
        emit newForumChanged(m_newForum);
    }
}

void SubscriptionManagement::resetForumList()
{
    qDeleteAll(m_forumList);
    m_forumList.clear();
    emit forumListChanged();
}

void SubscriptionManagement::subscribeForumFinished(ForumSubscription *sub, bool success) {
    Q_UNUSED(sub);
    if (!success) emit showError("Subscription to forum failed. Please check network connection.");
}

void SubscriptionManagement::probeResults(ForumSubscription *probedSub) {
    m_probeInProgress = false;
    emit probeInProgressChanged(m_probeInProgress);
    if(!probedSub) {
        resetNewForum();
        emit showError("Sorry, no supported forum found at this URL");
    } else {
        m_newForum = ForumSubscription::newForProvider(probedSub->provider(), 0, true);
        m_newForum->copyFrom(probedSub);

        if(m_newForum->id()) {
            emit newForumChanged(m_newForum);
        } else {
            // Not found, adding
            connect(m_protocol, SIGNAL(forumGot(ForumSubscription*)), this, SLOT(newForumAdded(ForumSubscription*)));
            m_protocol->addForum(m_newForum);
        }
    }
}

void SubscriptionManagement::newForumAdded(ForumSubscription *sub) {
    if(sub) {
        Q_ASSERT(sub->id());
        m_newForum->copyFrom(sub);
    } else {
        emit showError("Can't subscibe this forum. Check url!");
    }
    emit newForumChanged(m_newForum);
}
