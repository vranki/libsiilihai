/*
 * forumthread.h
 *
 *  Created on: Oct 1, 2009
 *      Author: vranki
 */

#ifndef FORUMTHREAD_H_
#define FORUMTHREAD_H_
#include <QString>

class ForumThread {
public:
	ForumThread();
	virtual ~ForumThread();
	int forumid;
	QString groupid;
	QString id;
	int ordernum;
	QString name;
	QString lastchange;
	int changeset;
	QString toString() const;
	bool isSane() const;
};

#endif /* FORUMTHREAD_H_ */
