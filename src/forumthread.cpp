/*
 * forumthread.cpp
 *
 *  Created on: Oct 1, 2009
 *      Author: vranki
 */

#include "forumthread.h"

ForumThread::ForumThread() {
	// TODO Auto-generated constructor stub

}

ForumThread::~ForumThread() {
	// TODO Auto-generated destructor stub
}

QString ForumThread::toString() const {
	return forumid + "/" + groupid + "/" + id + ": " + name;
}

bool ForumThread::isSane() const {
	return (forumid >=0 && groupid.length() > 0 && name.length() > 0 && id.length() > 0);
}
