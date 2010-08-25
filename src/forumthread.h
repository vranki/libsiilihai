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
#ifndef FORUMTHREAD_H_
#define FORUMTHREAD_H_
#include <QString>
#include <QObject>
#include <QList>
#include "forumgroup.h"

class ForumMessage;

class ForumThread : public QObject {
    Q_OBJECT
public:
    ForumThread(ForumGroup *grp);
    virtual ~ForumThread();

    void copyFrom(ForumThread * o);
  //  ForumThread(const ForumThread&);

    QString id() const;
    int ordernum() const;
    QString name() const;
    QString lastchange() const;
    int changeset() const;
    ForumGroup *group() const;
    bool hasMoreMessages() const;
    int getMessagesCount() const; // Max number of messages to get
    int unreadCount() const;

    void setId(QString nid);
    void setOrdernum(int on);
    void setName(QString nname);
    void setLastchange(QString lc);
    void setChangeset(int cs);
    void setHasMoreMessages(bool hmm);
    void setGetMessagesCount(int gmc);
    void setGroup(ForumGroup *ng);
    void incrementUnreadCount(int urc);
    QString toString() const;
    bool isSane() const;
    QList<ForumMessage*> & messages();
signals:
    void changed(ForumThread *thr);
    void unreadCountChanged(ForumThread *thr);

private:
    Q_DISABLE_COPY(ForumThread);
    int _forumid;
    QString _groupid;
    QString _id;
    int _ordernum;
    QString _name;
    QString _lastchange;
    int _changeset;
    ForumGroup *_group;
    bool _hasMoreMessages;
    int _getMessagesCount;
    bool _hasChanged;
    int _unreadCount;
    QList<ForumMessage*> _messages;
};

#endif /* FORUMTHREAD_H_ */
