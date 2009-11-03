/*
 * forumsubscription.cpp
 *
 *  Created on: Sep 27, 2009
 *      Author: vranki
 */

#include "forumgroup.h"

ForumGroup::ForumGroup() {
	id = QString::null;
	subscribed = false;
}

ForumGroup::~ForumGroup() {
}

QString ForumGroup::toString() const {
	return QString().number(parser) + "/" + id + ": " + name;
}
bool ForumGroup::isSane() const {
	return (id.length() > 0 && name.length() > 0);
}
