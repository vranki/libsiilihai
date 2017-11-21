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
#include <QMap>
#include <QMultiMap>

#include "forumdataitem.h"

class ForumMessage;
class ForumGroup;

/**
  * Represents a single thread in a forum. Contains a list of ForumMessages.
  *
  * @see ForumMessage
  */
class ForumThread : public ForumDataItem, public QMap<QString, ForumMessage*> {
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY changed)
    Q_PROPERTY(QString displayName READ displayName NOTIFY changed)
    Q_PROPERTY(QString id READ id WRITE setId NOTIFY changed)
    Q_PROPERTY(int unreadCount READ unreadCount() NOTIFY unreadCountChanged)
    Q_PROPERTY(bool hasMoreMessages READ hasMoreMessages() NOTIFY changed)
    Q_PROPERTY(QList<QObject*> messages READ messages NOTIFY messagesChanged)
    Q_PROPERTY(int ordernum READ ordernum WRITE setOrdernum NOTIFY changed)

public:
    ForumThread(QObject *parent, bool temp=true);
    virtual ~ForumThread();
    bool operator<(const ForumThread &o);
    void copyFrom(ForumThread * o);
    int ordernum() const;
    int changeset() const;
    ForumGroup *group() const;
    void setGroup(ForumGroup * grp);
    bool hasMoreMessages() const;
    int getMessagesCount() const; // Max number of messages to get
    void setOrdernum(int on);
    void setChangeset(int cs);
    void setHasMoreMessages(bool hmm);
    void setGetMessagesCount(int gmc);
    void setLastPage(int lp);
    int lastPage();
    virtual QString toString() const;
    bool isSane() const;
    bool isTemp();
    void addMessage(ForumMessage* msg, bool affectsSync = true);
    void removeMessage(ForumMessage* msg, bool affectsSync = true);
    virtual bool needsToBeUpdated() const;
    virtual void markToBeUpdated(bool toBe=true);

    QList<QObject *> messages() const;
signals:
    void changed();
    void unreadCountChanged();
    void messageRemoved(ForumMessage *msg);
    void messageAdded(ForumMessage *msg);
    void messagesChanged();
protected:
    virtual void emitChanged();
    virtual void emitUnreadCountChanged();
private:
    Q_DISABLE_COPY(ForumThread);
    int _ordernum;
    QString _lastchange;
    int _changeset;
    ForumGroup *_group;
    bool _hasMoreMessages;
    int _getMessagesCount;
    bool _temp;
    int _lastPage;
};

#endif /* FORUMTHREAD_H_ */
