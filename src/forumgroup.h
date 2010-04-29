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
#include "forumsubscription.h"

class ForumGroup : public QObject {
    Q_OBJECT

public:
    ForumGroup(ForumSubscription *sub);
    virtual ~ForumGroup();
    ForumGroup& operator=(const ForumGroup&);
    ForumGroup(const ForumGroup&);
    QString toString() const;
    bool isSane() const;
    ForumSubscription *subscription() const;
    QString name() const;
    QString id() const;
    QString lastchange() const;
    bool subscribed() const;
    int changeset() const;

    // void setSubscription(ForumSubscription *sub);
    void setName(QString name);
    void setId(QString id);
    void setLastchange(QString lc);
    void setSubscribed(bool s);
    void setChangeset(int cs);
private:
    ForumSubscription *_subscription;
    QString _name;
    QString _id;
    QString _lastchange;
    bool _subscribed;
    int _changeset;
};

#endif
