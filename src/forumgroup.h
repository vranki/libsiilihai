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
#include <QList>
#include "forumsubscription.h"
#include "forumdataitem.h"

class ForumThread;

class ForumGroup : public ForumDataItem {
    Q_OBJECT
public:
    ForumGroup(ForumSubscription *sub, bool temp=true);
    virtual ~ForumGroup();
    void copyFrom(ForumGroup * o);
    virtual QString toString() const;
    bool isSane() const;
    ForumSubscription *subscription() const;
    bool subscribed() const;
    int changeset() const;
    bool hasChanged() const;
    void setSubscribed(bool s);
    void setChangeset(int cs);
    void setHasChanged(bool hc);
    QMap<QString, ForumThread*> & threads();
    bool isTemp();

signals:
    void changed(ForumGroup *grp);
    void unreadCountChanged(ForumGroup *grp);
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

    QMap<QString, ForumThread*> _threads;
        bool _temp;
};

#endif
