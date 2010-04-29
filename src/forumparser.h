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

#ifndef FORUMPARSER_H_
#define FORUMPARSER_H_
#include <QString>

class ForumParser {
public:
	enum ForumLoginType { LoginTypeNotSupported=0, LoginTypeHttpPost=1, LoginTypeHttpAuth=2 };

	ForumParser();
	virtual ~ForumParser();
	QString toString();
	bool isSane() const;
	bool mayWork() const;
	QString forumUrlWithoutEnd() const;
	bool supportsThreadPages() const;
	bool supportsMessagePages() const;
	bool supportsMessageUrl() const;
	bool supportsLogin() const;
	// @todo these _should_ be private and have getters&setters
	int id;
	QString parser_name;
	QString forum_url;
	QString thread_list_path;
	QString view_thread_path;
	QString login_path;
	int date_format;
	QString group_list_pattern;
	QString thread_list_pattern;
	QString message_list_pattern;
	QString verify_login_pattern;
	QString login_parameters;
	ForumLoginType login_type;
	QString charset;
	int thread_list_page_start;
	int thread_list_page_increment;
	int view_thread_page_start;
	int view_thread_page_increment;
	QString forum_software;
	QString view_message_path;
	int parser_type;
	int parser_status;
	QString posting_path;
	QString posting_subject;
	QString posting_message;
	QString posting_parameters;
	QString posting_hints;
	QString reply_path;
	QString reply_subject;
	QString reply_message;
	QString reply_parameters;
private:
};

#endif /* FORUMPARSER_H_ */
