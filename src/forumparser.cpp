/* This file is part of libSiilihai.

    libSiilihai is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libSiilihai is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with libSiilihai.  If not, see <http://www.gnu.org/licenses/>. */
#include "forumparser.h"

ForumParser::ForumParser() {
	id = -1;
	thread_list_page_start = 0;
	thread_list_page_increment = 0;
	view_thread_page_start = 0;
	view_thread_page_increment = 0;
	login_type = LoginTypeNotSupported;
	parser_type = 0;
	parser_status = 0;
}

ForumParser::~ForumParser() {
}

QString ForumParser::toString() {
	return QString().number(id) + ": " + parser_name;
}

bool ForumParser::isSane() const {
	return mayWork() && id > 0;
}

bool ForumParser::mayWork() const {
	// @todo check all required fields
	return parser_name.length() > 3 && forum_url.length() >4 && thread_list_path.length() > 2;
}

QString ForumParser::forumUrlWithoutEnd() const {
	int i = forum_url.lastIndexOf('/');
	if (i > 0) {
		return forum_url.left(i + 1);
	} else {
		return forum_url;
	}
}

bool ForumParser::supportsThreadPages() const {
	return (thread_list_path.contains("%p"));
}

bool ForumParser::supportsMessagePages() const {
	return (view_thread_path.contains("%p"));
}

bool ForumParser::supportsMessageUrl() const {
	return (view_message_path.size() > 0);
}

bool ForumParser::supportsLogin() const {
	return (login_path.length() > 0 && login_type > 0);
}
