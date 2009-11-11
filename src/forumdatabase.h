#ifndef FORUMDATABASE_H_
#define FORUMDATABASE_H_
#include <QObject>
#include <QtSql>
#include <QList>
#include "forumgroup.h"
#include "forumthread.h"
#include "forumsubscription.h"
#include "forummessage.h"

class ForumDatabase : public QObject {
	Q_OBJECT
public:
	ForumDatabase(QObject *parent);
	virtual ~ForumDatabase();
	bool openDatabase( );
	bool addForum(const ForumSubscription &fs);
	bool deleteForum(const int forumid);
	QList <ForumSubscription> listSubscriptions();
	ForumSubscription getSubscription(int id);
	QList <ForumGroup> listGroups(const int parser);
	QList <ForumThread> listThreads(const ForumGroup &group);
	QList <ForumMessage> listMessages(const ForumThread &thread);
	bool addThread(const ForumThread &thread);
	bool deleteThread(const ForumThread &thread);
	bool addMessage(const ForumMessage &message);
	bool updateMessage(const ForumMessage &message);
	bool deleteMessage(const ForumMessage &message);
	bool addGroup(const ForumGroup &grp);
	bool updateGroup(const ForumGroup &grp);
	bool deleteGroup(const ForumGroup &grp);
	int unreadIn(const ForumSubscription &fs);
	int unreadIn(const ForumGroup &fg);
	bool markForumRead(const int forumid, bool read);
	bool markGroupRead(const ForumGroup &group, bool read);
public slots:
	bool markMessageRead(const ForumMessage &message);
	bool markMessageRead(const ForumMessage &message, bool read);
private:
	void bindMessageValues(QSqlQuery &query, const ForumMessage &message);
};

#endif /* FORUMDATABASE_H_ */
