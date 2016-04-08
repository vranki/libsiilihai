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

#ifndef FORUMGROUP_H_
#define FORUMGROUP_H_
#include <QString>
#include <QObject>
#include <QMap>
#include "forumdataitem.h"

class ForumSubscription;
class ForumThread;
/**
  * Represents a single thread in a forum. Contains threads.
  */
class ForumGroup : public ForumDataItem, public QMap<QString, ForumThread*> {
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY changed) // Prefer displayName in UI
    // This contains the parent groups of the group (if known)
    Q_PROPERTY(QString hierarchy READ hierarchy WRITE setHierarchy NOTIFY changed)
    Q_PROPERTY(QString displayName READ displayName NOTIFY changed)
    Q_PROPERTY(QString id READ id WRITE setId NOTIFY changed)
    Q_PROPERTY(int unreadCount READ unreadCount NOTIFY unreadCountChanged)
    Q_PROPERTY(bool isSubscribed READ isSubscribed WRITE setSubscribed NOTIFY changed)
    Q_PROPERTY(QList<QObject*> threads READ threads NOTIFY threadsChanged)

public:
    ForumGroup(QObject *parent, bool temp=true);
    virtual ~ForumGroup();
    void copyFrom(ForumGroup * o);
    virtual QString toString() const;
    bool isSane() const;
    ForumSubscription *subscription() const;
    bool isSubscribed() const;
    int changeset() const;
    bool hasChanged() const;
    bool isTemp();
    virtual void markToBeUpdated(bool toBe=true);
    virtual void incrementUnreadCount(int urc);
    QString hierarchy() const;
    QList<QObject*> threads() const;

public slots:
    void setHierarchy(QString arg);
    void setSubscription(ForumSubscription *sub);
    void setSubscribed(bool s);
    void setChangeset(int cs);
    void setHasChanged(bool hc);
    void addThread(ForumThread* thr, bool affectsSync = true, bool incrementUnreads = true);
    void removeThread(ForumThread* thr, bool affectsSync = true);
    void markRead(bool read=true);

signals:
    void changed();
    void unreadCountChanged();
    void threadRemoved(ForumThread *thr);
    void threadAdded(ForumThread *thr);
    void threadsChanged();

protected:
    virtual void emitChanged();
    virtual void emitUnreadCountChanged();
private:
    Q_DISABLE_COPY(ForumGroup);
    ForumSubscription *_subscription;
    QString _lastchange;
    QString _hierarchy;
    bool _subscribed;
    int _changeset;
    bool _hasChanged;
    bool _temp;
    QList<QObject*> m_threads;
};

#endif
