#include "forumthread.h"

ForumThread::ForumThread() {
	forumid = -1;
	id = QString::null;
}

ForumThread::~ForumThread() {
}

QString ForumThread::toString() const {
	return QString().number(forumid) + "/" + groupid + "/" + id + ": " + name;
}

bool ForumThread::isSane() const {
	return (groupid.length() > 0 && name.length() > 0 && id.length() > 0);
}
