#include "subscriptionmanagement.h"
#include "siilihaiprotocol.h"
#include "siilihaisettings.h"

SubscriptionManagement::SubscriptionManagement(QObject *parent, SiilihaiProtocol *protocol, SiilihaiSettings *settings)
    : QObject(parent), m_protocol(protocol), m_newForum(0), m_probe(0, m_protocol), m_settings(settings)
{
    connect(m_protocol, SIGNAL(listForumsFinished(QList <ForumSubscription*>)), this, SLOT(listForumsFinished(QList <ForumSubscription*>)));
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
        if (aliasLc.contains(m_forumFilter)
                || urlLc.contains(m_forumFilter)) {
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
    qDeleteAll(m_forumList);
    m_forumList.clear();
    m_forumList.append(forums);
    emit forumListChanged();
}

void SubscriptionManagement::listForums()
{
    resetNewForum();
    m_protocol->listForums();
}

void SubscriptionManagement::unsubscribeForum(ForumSubscription *fs)
{
    emit forumUnsubscribed(fs);
}

void SubscriptionManagement::getForum(int id)
{
    emit showError(QString::null);
    m_probe.probeUrl(id);
}

void SubscriptionManagement::getForum(QUrl url)
{
    emit showError(QString::null);
    m_probe.probeUrl(url);
}

void SubscriptionManagement::subscribeThisForum(QString user, QString pass)
{
    if(!m_newForum) return;

    m_newForum->setAuthenticated(user.length() > 0);
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
    emit showError(QString::null);
    if(m_newForum) {
        m_newForum->deleteLater();
        m_newForum = 0;
    }
    emit newForumChanged(m_newForum);
}

void SubscriptionManagement::subscribeForumFinished(ForumSubscription *sub, bool success) {
    Q_UNUSED(sub);
    if (!success) {
        emit showError("Subscribing to forum failed. Please check network connection.");
    }
}

void SubscriptionManagement::probeResults(ForumSubscription *probedSub) {
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
        emit newForumChanged(m_newForum);
    } else {
        emit showError("Can't subscibe this forum. Check url!");
    }
}
