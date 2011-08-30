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

ForumParser::ForumParser(QObject *parent) : QObject(parent){
    id = -1;
    thread_list_page_start = 0;
    thread_list_page_increment = 0;
    view_thread_page_start = 0;
    view_thread_page_increment = 0;
    login_type = LoginTypeNotSupported;
    parser_type = 0;
    parser_status = 0;
}

ForumParser &ForumParser::operator=(const ForumParser& o) {
    id = o.id;
    parser_name = o.parser_name;
    forum_url = o.forum_url;
    thread_list_path = o.thread_list_path;
    view_thread_path = o.view_thread_path;
    login_path = o.login_path;
    date_format = o.date_format;
    group_list_pattern = o.group_list_pattern;
    thread_list_pattern = o.thread_list_pattern;
    message_list_pattern = o.message_list_pattern;
    verify_login_pattern = o.verify_login_pattern;
    login_parameters = o.login_parameters;
    login_type = o.login_type;
    charset = o.charset;
    thread_list_page_start = o.thread_list_page_start;
    thread_list_page_increment = o.thread_list_page_increment;
    view_thread_page_start = o.view_thread_page_start;
    view_thread_page_increment = o.view_thread_page_increment;
    forum_software = o.forum_software;
    view_message_path = o.view_message_path;
    parser_type = o.parser_type;
    parser_status = o.parser_status;
    posting_path = o.posting_path;
    posting_subject = o.posting_subject;
    posting_message = o.posting_message;
    posting_parameters = o.posting_parameters;
    posting_hints = o.posting_hints;
    reply_path = o.reply_path;
    reply_subject = o.reply_subject;
    reply_message = o.reply_message;
    reply_parameters = o.reply_parameters;
    return *this;
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
