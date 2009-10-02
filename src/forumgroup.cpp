/*
 * forumsubscription.cpp
 *
 *  Created on: Sep 27, 2009
 *      Author: vranki
 */

#include "forumgroup.h"

ForumGroup::ForumGroup() {
	// TODO Auto-generated constructor stub
	id = QString::null;
	subscribed = false;
}

ForumGroup::~ForumGroup() {
	// TODO Auto-generated destructor stub
}

QString ForumGroup::toString() const {
	return id + ": " + name;
}
bool ForumGroup::isSane() const {
	return (parser > 0 && id.length() > 0 && name.length() > 0);
}
