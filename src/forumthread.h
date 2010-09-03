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
#include "forumdataitem.h"
//#include "forummessage.h"
#include "forumgroup.h"

class ForumMessage;
//class ForumGroup : public ForumDataItem;

class ForumThread : public ForumDataItem {
    Q_OBJECT
public:
    ForumThread(ForumGroup *grp, bool temp=true);
    virtual ~ForumThread();
    bool operator<(const ForumThread &o);
    void copyFrom(ForumThread * o);
    int ordernum() const;
    int changeset() const;
    ForumGroup *group() const;
    bool hasMoreMessages() const;
    int getMessagesCount() const; // Max number of messages to get
    void setOrdernum(int on);
    void setChangeset(int cs);
    void setHasMoreMessages(bool hmm);
    void setGetMessagesCount(int gmc);
    //void setGroup(ForumGroup *ng);
    virtual QString toString() const;
    bool isSane() const;
    QMap<QString, ForumMessage*> & messages();
    bool isTemp();
signals:
    void changed(ForumThread *thr);
    void unreadCountChanged(ForumThread *thr);
protected:
    virtual void emitChanged();
    virtual void emitUnreadCountChanged();
private:
    Q_DISABLE_COPY(ForumThread);
    int _forumid;
    QString _groupid;
    int _ordernum;
    QString _lastchange;
    int _changeset;
    ForumGroup *_group;
    bool _hasMoreMessages;
    int _getMessagesCount;
    bool _hasChanged;
    QMap<QString, ForumMessage*> _messages;
        bool _temp;
};

#endif /* FORUMTHREAD_H_ */
