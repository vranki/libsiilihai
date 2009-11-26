#include "forumthread.h"

ForumThread::ForumThread() {
	forumid = -1;
	id = QString::null;
	changeset = -1;
}

ForumThread::~ForumThread() {
}

QString ForumThread::toString() const {
	return QString().number(forumid) + "/" + groupid + "/" + id + ": " + name;
}

bool ForumThread::isSane() const {
	return (groupid.length() > 0 && forumid > 0 && id.length() > 0);
}
