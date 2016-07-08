#ifndef SUBSCRIPTIONMANAGEMENT_H
#define SUBSCRIPTIONMANAGEMENT_H

#include <QObject>
#include <QList>
#include "forumdata/forumsubscription.h"
#include "forumprobe.h"

class SiilihaiProtocol;
class SiilihaiSettings;

/**
 * @brief The SubscriptionManagement class manages subscribing a new forum
 * To subscribe:
 * - Create SubscriptionManagement.
 * - Use listForums() to update forumList which contains list of known forums.
 * - Use getForum() to get the forum details to subscribe.
 * - Wait for newForumChanged(). After that newForum contains this forum or null.
 * - Use subscribeThisForum() to actually subscribe the current forum in newForum.
 *
 * To unsubscribe:
 * - Use unsubscribeForum()
 */
class SubscriptionManagement : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QObject*> forumList READ forumList NOTIFY forumListChanged)
    Q_PROPERTY(QString forumFilter READ forumFilter WRITE setForumFilter NOTIFY forumFilterChanged)
    Q_PROPERTY(ForumSubscription* newForum READ newForum NOTIFY newForumChanged)

public:
    explicit SubscriptionManagement(QObject *parent, SiilihaiProtocol *protocol, SiilihaiSettings *settings);
    ~SubscriptionManagement();
    Q_INVOKABLE virtual void listForums();
    Q_INVOKABLE virtual void unsubscribeForum(ForumSubscription* fs);
    Q_INVOKABLE virtual void getForum(int id);
    Q_INVOKABLE virtual void getForum(QUrl url);
    Q_INVOKABLE virtual void subscribeThisForum(QString user, QString pass);
    Q_INVOKABLE virtual void resetNewForum(); // Sets new forum to null
    Q_INVOKABLE virtual void resetForumList(); // Clears the forum list, freeing up resources

    // List of (incomplete) forums for subscribing. Use listForums() to update.
    QList<QObject*> forumList();
    QString forumFilter() const;
    ForumSubscription* newForum() const;

public slots:
    void setForumFilter(QString forumFilter);

signals:
    void forumListChanged();
    void showError(QString errorText);
    void forumUnsubscribed(ForumSubscription *fs);
    void forumFilterChanged(QString forumFilter);
    void forumAdded(ForumSubscription *fs); // To clientlogic
    void newForumChanged(ForumSubscription* newForum);

private slots:
    void listForumsFinished(QList <ForumSubscription*>);
    void subscribeForumFinished(ForumSubscription *sub, bool success);
    void probeResults(ForumSubscription *probedSub);
    void newForumAdded(ForumSubscription *sub);

private:

    SiilihaiProtocol *m_protocol;
    QList<ForumSubscription*> m_forumList;
    ForumSubscription *m_newForum;
    ForumProbe m_probe;
    QString m_forumFilter;
    SiilihaiSettings *m_settings;
};

#endif // SUBSCRIPTIONMANAGEMENT_H
