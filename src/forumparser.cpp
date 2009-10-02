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

QString ForumParser::forumUrlWithoutEnd() const {
	int i = forum_url.lastIndexOf('/');
	if(i>0) {
	return forum_url.left(i+1);
	} else {
		return forum_url;
	}
}

bool ForumParser::supportsThreadPages() const {
	return (thread_list_path.contains("%p") && thread_list_page_start >= 0 && thread_list_page_increment != 0);
}

bool ForumParser::supportsMessaegePages() const {
	return (view_thread_path.contains("%p") && view_thread_page_start >= 0 && view_thread_page_increment != 0);
}
