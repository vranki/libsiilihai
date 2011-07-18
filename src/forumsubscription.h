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
#include <QMap>
#include <QList>
#include <QDebug>
#include "updateableitem.h"

class ForumGroup;
class ParserEngine;

/**
  * Represents a subscription to a forum. Contains a list of
  * groups.
  *
  * @see ForumGroup
  */
class ForumSubscription : public QObject, public QMap<QString, ForumGroup*>, public UpdateableItem  {
    Q_OBJECT

public:
    ForumSubscription(QObject *parent=0, bool temp=true);
    void copyFrom(ForumSubscription * o);
    virtual ~ForumSubscription();
    bool isSane() const;
    QString toString() const;
    void setParser(int parser);
    void setAlias(QString alias);
    void setUsername(QString username);
    void setPassword(QString password);
    void setLatestThreads(unsigned int lt);
    void setLatestMessages(unsigned int lm);
    void setAuthenticated(bool na);
    void incrementUnreadCount(int urc);
    void addGroup(ForumGroup* grp, bool affectsSync=true);
    void removeGroup(ForumGroup* grp, bool affectsSync=true);
    void setGroupListChanged(bool changed=true); // To trigger sending group list update

    int parser() const;
    QString alias() const;
    QString username() const;
    QString password() const;
    int latestThreads() const;
    int latestMessages() const;
    bool authenticated() const; // True if username & password should be set
    int unreadCount() const;
    bool isTemp() const;
    void setParserEngine(ParserEngine *eng);
    ParserEngine *parserEngine() const;
    bool hasGroupListChanged() const;
    void markRead(bool read=true);
 signals:
    void changed(ForumSubscription *s);
    void unreadCountChanged(ForumSubscription *s);
    void groupRemoved(ForumGroup *grp);
    void groupAdded(ForumGroup *grp);

private:
    Q_DISABLE_COPY(ForumSubscription)

    int _parser;
    QString _alias;
    QString _username;
    QString _password;
    unsigned int _latestThreads;
    unsigned int _latestMessages;
    bool _authenticated;
    int _unreadCount;
    bool _temp, _groupListChanged;
    ParserEngine *_engine;
};

#endif /* FORUMSUBSCRIPTION_H_ */
