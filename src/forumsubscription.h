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
class ForumGroup;

class ForumSubscription : public QObject, public QList<ForumGroup*> {
	Q_OBJECT

public:
//	ForumSubscription();
        ForumSubscription(QObject *parent=0);
        ForumSubscription(const ForumSubscription&);
        ForumSubscription&  operator=(const ForumSubscription&);
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
	int parser() const;
        QString alias() const;
	QString username() const;
	QString password() const;
	unsigned int latest_threads() const;
	unsigned int latest_messages() const;
        bool authenticated() const; // True if username & password should be set
        int unreadCount() const;
signals:
    void changed(ForumSubscription *s);
    void unreadCountChanged(ForumSubscription *s);

private:
	int _parser;
        QString _alias;
	QString _username;
	QString _password;
	unsigned int _latestThreads;
	unsigned int _latestMessages;
        bool _authenticated;
        int _unreadCount;
};

#endif /* FORUMSUBSCRIPTION_H_ */
