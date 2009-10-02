/*
 * forummessage.cpp
 *
 *  Created on: Oct 2, 2009
 *      Author: vranki
 */

#include "forummessage.h"

ForumMessage::ForumMessage() {
	// TODO Auto-generated constructor stub

}

ForumMessage::~ForumMessage() {
	// TODO Auto-generated destructor stub
}

bool ForumMessage::isSane() const {
	return (forumid>0&&groupid.length()>0&&threadid.length()>0&&id.length()>0);
}

QString ForumMessage::toString() const {
	return forumid + "/" + groupid + "/" + threadid + "/" + id + ": " + subject;
}
