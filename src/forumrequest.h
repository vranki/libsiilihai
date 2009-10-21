/*
 * forumrequest.h
 *
 *  Created on: Oct 16, 2009
 *      Author: vranki
 */

#ifndef FORUMREQUEST_H_
#define FORUMREQUEST_H_
#include <QString>

class ForumRequest {
public:
	ForumRequest();
	virtual ~ForumRequest();
	QString forum_url;
	QString date;
	QString user;
	QString comment;
};

#endif /* FORUMREQUEST_H_ */
