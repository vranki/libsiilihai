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
	void setSubscription(ForumSubscription *fs);
	void updateGroupList();
	void updateForum(bool force=false);
	bool isBusy();
public slots:
	void listMessagesFinished(QList<ForumMessage> &messages, ForumThread *thread);
	void listGroupsFinished(QList<ForumGroup> &groups);
	void listThreadsFinished(QList<ForumThread> &threads, ForumGroup *group);
	void networkFailure(QString message);
	void cancelOperation();
signals:
	void groupListChanged(ForumSubscription *forum);
	void forumUpdated(ForumSubscription *forum);
	void statusChanged(ForumSubscription *forum, bool reloading, float progress);
	void updateFailure(QString message);
private:
	void updateNextChangedGroup();
	void updateNextChangedThread();
	void setBusy(bool busy);
	void updateCurrentProgress();
	ForumParser parser;
	ForumSubscription *subscription;
	ForumSession session;
	bool sessionInitialized;
	bool updateAll;
	bool forumBusy;
	bool forceUpdate; // Update even if no changes
	ForumDatabase *fdb;
	QQueue<ForumGroup*> groupsToUpdateQueue;
	QQueue<ForumThread*> threadsToUpdateQueue;
//	int largestGroupsToUpdateQueue, largestThreadsToUpdateQueue;
};

#endif /* PARSERENGINE_H_ */
