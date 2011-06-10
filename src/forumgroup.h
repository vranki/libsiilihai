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
public:
    ForumGroup(ForumSubscription *sub, bool temp=true);
    virtual ~ForumGroup();
    void copyFrom(ForumGroup * o);
    virtual QString toString() const;
    bool isSane() const;
    ForumSubscription *subscription() const;
    bool isSubscribed() const;
    int changeset() const;
    bool hasChanged() const;
    void setSubscribed(bool s);
    void setChangeset(int cs);
    void setHasChanged(bool hc);
    bool isTemp();
    void markToBeUpdated();
    void addThread(ForumThread* thr, bool affectsSync = true);
    void removeThread(ForumThread* thr, bool affectsSync = true);

signals:
    void changed(ForumGroup *grp);
    void unreadCountChanged(ForumGroup *grp);
    void threadRemoved(ForumThread *thr);
    void threadAdded(ForumThread *thr);
protected:
    virtual void emitChanged();
    virtual void emitUnreadCountChanged();
private:
    Q_DISABLE_COPY(ForumGroup);
    ForumSubscription *_subscription;
    QString _lastchange;
    bool _subscribed;
    int _changeset;
    bool _hasChanged;
    bool _temp;
};

#endif
