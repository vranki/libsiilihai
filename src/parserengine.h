/*
 * parserengine.h
 *
 *  Created on: Sep 18, 2009
 *      Author: vranki
 */

#ifndef PARSERENGINE_H_
#define PARSERENGINE_H_
#include <QObject>
#include "forumparser.h"
#include "forumsubscription.h"
#include "forumsession.h"
#include "forumgroup.h"
#include "forummessage.h"
#include "forumdatabase.h"

class ParserEngine : public QObject {
	Q_OBJECT

public:
	ParserEngine(ForumDatabase *fd, QObject *parent=0);
	virtual ~ParserEngine();
	void setParser(ForumParser &fp);
	void setSubscription(ForumSubscription &fs);
	void updateGroupList();
	void updateForum();
public slots:
	void listMessagesFinished(QList<ForumMessage> messages, ForumThread thread);
	void listGroupsFinished(QList<ForumGroup> groups);
	void listThreadsFinished(QList<ForumThread> threads, ForumGroup group);
signals:
	void groupListChanged(int forum);
	void forumUpdated(int forum);
	void statusChanged(int forum, bool reloading);
private:
	void updateNextChangedGroup();
	void updateNextChangedThread();
	ForumParser parser;
	ForumSubscription subscription;
	ForumSession session;
	bool sessionInitialized;
	bool updateAll;
	ForumDatabase *fdb;
	QQueue<ForumGroup> groupsToUpdateQueue;
	QQueue<ForumThread> threadsToUpdateQueue;
};

#endif /* PARSERENGINE_H_ */
