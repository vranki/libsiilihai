/*
 * forumsubscription.h
 *
 *  Created on: Sep 27, 2009
 *      Author: vranki
 */

#ifndef FORUMSUBSCRIPTION_H_
#define FORUMSUBSCRIPTION_H_
#include <QString>

class ForumSubscription {
public:
	ForumSubscription();
	virtual ~ForumSubscription();
	bool isSane() const;
	int parser;
	QString name;
	QString username;
	QString password;
	int latest_threads;
	int latest_messages;
};

#endif /* FORUMSUBSCRIPTION_H_ */
