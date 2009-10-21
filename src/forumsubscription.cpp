/*
 * forumsubscription.cpp
 *
 *  Created on: Sep 27, 2009
 *      Author: vranki
 */

#include "forumsubscription.h"

ForumSubscription::ForumSubscription() {
	parser = -1;
}

ForumSubscription::~ForumSubscription() {
}

bool ForumSubscription::isSane() const {
	return (parser > 0 && name.length() > 0 && latest_messages > 0 && latest_threads > 0);
}

QString ForumSubscription::toString() const {
	return QString("Subscription to ") + QString().number(parser) + " (" + name + ")";
}
