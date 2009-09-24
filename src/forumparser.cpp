/*
 * forumparser.cpp
 *
 *  Created on: Sep 18, 2009
 *      Author: vranki
 */

#include "forumparser.h"

ForumParser::ForumParser() {
	// TODO Auto-generated constructor stub
	id = -1;
}

ForumParser::~ForumParser() {
	// TODO Auto-generated destructor stub
}

QString ForumParser::toString() {
	return QString().number(id) + ": " + parser_name;
}

bool ForumParser::isSane() const {
	return parser_name.length()>3 && id > 0;
}
