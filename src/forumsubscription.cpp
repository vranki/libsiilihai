/*
 * forumsubscription.cpp
 *
 *  Created on: Sep 27, 2009
 *      Author: vranki
 */

#include "forumsubscription.h"

ForumSubscription::ForumSubscription() {
	// TODO Auto-generated constructor stub

}

ForumSubscription::~ForumSubscription() {
	// TODO Auto-generated destructor stub
}

bool ForumSubscription::isSane() const {
	return (parser > 0 && name.length() > 0 && latest_messages > 0 && latest_threads > 0);
}
