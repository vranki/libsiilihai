/*
 * forummessage.cpp
 *
 *  Created on: Oct 2, 2009
 *      Author: vranki
 */

#include "forummessage.h"

ForumMessage::ForumMessage() {
	forumid = -1;
	id = QString::null;
}

ForumMessage::~ForumMessage() {
}

bool ForumMessage::isSane() const {
	return (groupid.length()>0&&threadid.length()>0&&id.length()>0);
}

QString ForumMessage::toString() const {
	return QString().number(forumid) + "/" + groupid + "/" + threadid + "/" + id + ": " + subject;
}
