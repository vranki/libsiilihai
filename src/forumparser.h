/*
 * forumparser.h
 *
 *  Created on: Sep 18, 2009
 *      Author: vranki
 */

#ifndef FORUMPARSER_H_
#define FORUMPARSER_H_
#include <QString>

class ForumParser {
public:
	ForumParser();
	virtual ~ForumParser();
	// @todo these _should_ be private
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
	int login_type;
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
	QString toString();
	bool isSane() const;
	QString forumUrlWithoutEnd() const;
	bool supportsThreadPages() const;
	bool supportsMessaegePages() const;
private:
};

#endif /* FORUMPARSER_H_ */
