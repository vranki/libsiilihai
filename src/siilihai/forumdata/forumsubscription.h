/* This file is part of libSiilihai.

    libSiilihai is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libSiilihai is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with libSiilihai.  If not, see <http://www.gnu.org/licenses/>. */
#ifndef FORUMSUBSCRIPTION_H_
#define FORUMSUBSCRIPTION_H_
#include <QString>
#include <QObject>
#include <QList>
#include <QDebug>
#include <QUrl>
#include <QDomElement>
#include <QDomDocument>
#include <QtQml/QQmlListProperty>
#include "updateableitem.h"
#include "updateerror.h"

class ForumGroup;
class UpdateEngine;

#define SUB_SUBSCRIPTION "subscription"
#define SUB_PROVIDER "provider"
#define SUB_FORUMID "forumid"
#define SUB_ALIAS "alias"
#define SUB_USERNAME "username"
#define SUB_PASSWORD "password"
#define SUB_LATEST_THREADS "latest_threads"
#define SUB_LATEST_MESSAGES "latest_messages"

/**
  * Represents a subscription to a forum. Contains a list of groups.
  *
  * @see ForumGroup
  */
class ForumSubscription : public QObject, public QList<ForumGroup*>, public UpdateableItem  {
    Q_OBJECT
    Q_PROPERTY(int id READ id WRITE setId NOTIFY changed)
    Q_PROPERTY(QString alias READ alias WRITE setAlias NOTIFY changed)
    Q_PROPERTY(int unreadCount READ unreadCount NOTIFY unreadCountChanged)
    Q_PROPERTY(bool beingUpdated READ beingUpdated WRITE setBeingUpdated NOTIFY changed)
    Q_PROPERTY(bool beingSynced READ beingSynced WRITE setBeingSynced NOTIFY changed)
    Q_PROPERTY(bool scheduledForUpdate READ scheduledForUpdate WRITE setScheduledForUpdate NOTIFY changed)
    Q_PROPERTY(QUrl forumUrl READ forumUrl WRITE setForumUrl NOTIFY changed)
    Q_PROPERTY(bool supportsLogin READ supportsLogin WRITE setSupportsLogin NOTIFY changed)
    Q_PROPERTY(bool supportsPosting READ supportsPosting WRITE setSupportsPosting NOTIFY changed)
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY changed)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY changed)
    Q_PROPERTY(bool isAuthenticated READ isAuthenticated WRITE setAuthenticated NOTIFY changed)
    Q_PROPERTY(QUrl faviconUrl READ faviconUrl() NOTIFY changed)
    Q_PROPERTY(QQmlListProperty<UpdateError> errors READ errors NOTIFY errorsChanged)
    Q_PROPERTY(QList<QObject*> groups READ groups NOTIFY groupsChanged)
    // @todo doesn't follow changes yet
    Q_PROPERTY(QList<QObject*> subscribedGroups READ subscribedGroups NOTIFY groupsChanged)

public:
    enum ForumProvider {
        FP_NONE=0, // Error in practice..
        FP_PARSER,
        FP_TAPATALK,
        FP_DISCOURSE,
        FP_MAX
    };

    ForumSubscription(QObject *parent, bool temp, ForumProvider p);
    virtual void copyFrom(const ForumSubscription *o);
    virtual ~ForumSubscription();
    virtual bool isSane() const;
    QString toString() const;
    ForumGroup* value(const QString & id) const;
    bool contains(const QString & id) const;
    void setAlias(QString alias);
    void setUsername(QString username);
    void setPassword(QString password);
    void setLatestThreads(unsigned int lt);
    void setLatestMessages(unsigned int lm);
    void setAuthenticated(bool na); // @todo is this required? Why not just username.length() > 0 ?
    void incrementUnreadCount(int urc);
    void addGroup(ForumGroup* grp, bool affectsSync=true, bool incrementUnreads = true);
    void removeGroup(ForumGroup* grp, bool affectsSync=true, bool incrementUnreads = true);
    void setGroupListChanged(bool changed=true); // To trigger sending group list update
    int id() const;
    void setId(int newId);
    QString alias() const;
    QString username() const;
    QString password() const;
    int latestThreads() const;
    int latestMessages() const;
    bool isAuthenticated() const; // True if username & password should be set
    int unreadCount() const;
    bool isTemp() const;
    UpdateEngine *updateEngine() const; // Can be null!
    void engineDestroyed(); // Sets engine to 0
    bool hasGroupListChanged() const;
    Q_INVOKABLE void markRead(bool read=true);
    void setBeingUpdated(bool bu);
    void setBeingSynced(bool bs);
    void setScheduledForUpdate(bool su);
    void setScheduledForSync(bool su);
    bool beingUpdated() const;
    bool beingSynced() const;
    bool scheduledForSync() const;
    bool scheduledForUpdate() const;
    ForumProvider provider() const;
    QString providerName() const;

    static ForumSubscription *newForProvider(ForumProvider fp, QObject *parent, bool temp);
    virtual QDomElement serialize(QDomElement &parent, QDomDocument &doc);
    static ForumSubscription* readSubscription(QDomElement &element, QObject *parent);
    virtual void readSubscriptionXml(QDomElement &element);
    virtual QUrl forumUrl() const;
    void setForumUrl(QUrl url);
    void setSupportsLogin(bool sl);
    bool supportsLogin() const;
    void setSupportsPosting(bool sp);
    bool supportsPosting() const;
    QUrl faviconUrl() const;
    void setProvider(ForumProvider provider); // Use with care!!
    // Error list management
    QList<UpdateError *> errorList(); // Don't store
    void appendError(UpdateError *ue); // Ownership changes
    Q_INVOKABLE void clearErrors();
    QQmlListProperty<UpdateError> errors();
    QList<QObject*> groups() const;
    QList<QObject*> subscribedGroups() const;

signals:
    void changed();
    void unreadCountChanged();
    void engineChanged();
    void groupRemoved(ForumGroup *grp);
    void groupAdded(ForumGroup *grp);
    void aliasChanged(QString alias);
    void errorsChanged();
    void groupsChanged();

protected:
    UpdateEngine *_engine;

private:
    Q_DISABLE_COPY(ForumSubscription)
    static void append_error(QQmlListProperty<UpdateError> *list, UpdateError *msg);
    static int count_error(QQmlListProperty<UpdateError> *list);
    static UpdateError *at_error(QQmlListProperty<UpdateError> *list, int index);
    static void clear_error(QQmlListProperty<UpdateError> *list);

    int _forumId;
    QString _alias;
    QString _username;
    QString _password;
    unsigned int _latestThreads;
    unsigned int _latestMessages;
    // authenticated means forum authentication. HTTP authentication is
    // handled automatically!
    bool _authenticated, _supportsLogin, _supportsPosting;
    int _unreadCount;
    bool _temp, _groupListChanged;
    // Just for status display
    bool _scheduledForUpdate, _beingUpdated, _scheduledForSync, _beingSynced;
    ForumProvider _provider;
    QUrl _forumUrl;
    QList<UpdateError*> m_errors;
    QList<QObject*> m_subscribedGroups;
};

#endif /* FORUMSUBSCRIPTION_H_ */
