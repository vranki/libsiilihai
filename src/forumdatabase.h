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
	QList <ForumSubscription> listSubscriptions();
	QList <ForumGroup> listGroups(const int parser);
	// @todo strings to classes
	QList <ForumThread> listThreads(const ForumGroup &group);
	QList <ForumMessage> listMessages(const ForumThread &thread);
	bool addThread(const ForumThread &thread);
	bool deleteThread(const ForumThread &thread);
	bool addMessage(const ForumMessage &message);
	bool deleteMessage(const ForumMessage &message);
	bool addGroup(const ForumGroup &grp);
	bool updateGroup(const ForumGroup &grp);
	bool deleteGroup(const ForumGroup &grp);
};

#endif /* FORUMDATABASE_H_ */
