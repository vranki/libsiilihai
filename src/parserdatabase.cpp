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
#include "parserdatabase.h"

ParserDatabase::ParserDatabase(QObject *parent) :
	QObject(parent) {

}

ParserDatabase::~ParserDatabase() {

}

bool ParserDatabase::openDatabase( ) {
	QSqlQuery query;
	if (!query.exec("SELECT id FROM parsers")) {
		qDebug("DB doesn't exist, creating..");
		if (!query.exec("CREATE TABLE parsers (id INTEGER PRIMARY KEY, "
			"parser_name VARCHAR UNIQUE, "
			"forum_url VARCHAR, "
			"thread_list_path VARCHAR, "
			"view_thread_path VARCHAR, "
			"login_path VARCHAR, "
			"date_format INTEGER, "
			"group_list_pattern VARCHAR, "
			"thread_list_pattern VARCHAR, "
			"message_list_pattern VARCHAR, "
			"verify_login_pattern VARCHAR, "
			"login_parameters VARCHAR, "
			"login_type INTEGER, "
			"charset VARCHAR, "
			"thread_list_page_start INTEGER, "
			"thread_list_page_increment INTEGER, "
			"view_thread_page_start INTEGER, "
			"view_thread_page_increment INTEGER, "
			"forum_software VARCHAR,"
			"view_message_path VARCHAR,"
			"parser_type INTEGER, "
			"posting_path VARCHAR, "
			"posting_subject VARCHAR, "
			"posting_message VARCHAR, "
			"posting_parameters VARCHAR, "
			"posting_hints VARCHAR, "
			"reply_path VARCHAR, "
			"reply_subject VARCHAR, "
			"reply_message VARCHAR, "
			"reply_parameters VARCHAR"
			")")) {
			qDebug() << "Couldn't create parsers table!";
			return false;
		}
	}
	return true;
}
void ParserDatabase::deleteParser(const int id) {
	QSqlQuery query;
	query.prepare("DELETE FROM parsers WHERE(id=?)");
	query.addBindValue(id);
	query.exec();
}

bool ParserDatabase::storeParser(const ForumParser &p) {
	// @todo transaction could be used here
	if(!p.isSane()) {
		qDebug() << "Tried to store a insane parser!";
		return false;
	}

	QSqlQuery query;
	query.prepare("SELECT id FROM parsers WHERE id=?");
	query.addBindValue(p.id);
	if (query.exec()) {
		deleteParser(p.id);
	}
	query.prepare("INSERT INTO parsers("
		"id,"
		"parser_name,"
		"forum_url,"
		"thread_list_path,"
		"view_thread_path,"
		"login_path,"
		"date_format,"
		"group_list_pattern,"
		"thread_list_pattern,"
		"message_list_pattern,"
		"verify_login_pattern,"
		"login_parameters,"
		"login_type,"
		"charset, "
		"thread_list_page_start, "
		"thread_list_page_increment, "
		"view_thread_page_start, "
		"view_thread_page_increment, "
		"forum_software, "
		"view_message_path, "
		"parser_type, "
		"posting_path, "
		"posting_subject, "
		"posting_message, "
		"posting_parameters, "
		"posting_hints, "
		"reply_path, "
		"reply_subject, "
		"reply_message, "
		"reply_parameters)"

		" VALUES ("
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?, "
		"?"
		")");
	query.addBindValue(p.id);
	query.addBindValue(p.parser_name);
	query.addBindValue(p.forum_url);
	query.addBindValue(p.view_thread_path);
	query.addBindValue(p.thread_list_path);
	query.addBindValue(p.login_path);
	query.addBindValue(p.date_format);
	query.addBindValue(p.group_list_pattern);
	query.addBindValue(p.thread_list_pattern);
	query.addBindValue(p.message_list_pattern);
	query.addBindValue(p.verify_login_pattern);
	query.addBindValue(p.login_parameters);
	query.addBindValue(p.login_type);
	query.addBindValue(p.charset);
	query.addBindValue(p.thread_list_page_start);
	query.addBindValue(p.thread_list_page_increment);
	query.addBindValue(p.view_thread_page_start);
	query.addBindValue(p.view_thread_page_increment);
	query.addBindValue(p.forum_software);
	query.addBindValue(p.view_message_path);
	query.addBindValue(p.parser_type);
	query.addBindValue(p.posting_path);
	query.addBindValue(p.posting_subject);
	query.addBindValue(p.posting_message);
	query.addBindValue(p.posting_parameters);
	query.addBindValue(p.posting_hints);
	query.addBindValue(p.reply_path);
	query.addBindValue(p.reply_subject);
	query.addBindValue(p.reply_message);
	query.addBindValue(p.reply_parameters);

	if (!query.exec()) {
		qDebug() << "Adding parser failed: " << query.lastError().text();
		return false;
	}
	qDebug() << "Parser stored";
	return true;
}

ForumParser ParserDatabase::getParser(const int id) {
	ForumParser p;
	p.id = -1;

	QSqlQuery query;
	query.prepare("SELECT * FROM parsers WHERE(id=?)");
	query.addBindValue(id);
	if (query.exec()) {
		while (query.next()) {
			p.id = query.value(0).toInt();
			p.parser_name = query.value(1).toString();
			p.forum_url = query.value(2).toString();
			p.view_thread_path = query.value(3).toString();
			p.thread_list_path = query.value(4).toString();
			p.login_path = query.value(5).toString();
			p.date_format = query.value(6).toInt();
			p.group_list_pattern = query.value(7).toString();
			p.thread_list_pattern = query.value(8).toString();
			p.message_list_pattern = query.value(9).toString();
			p.verify_login_pattern = query.value(10).toString();
			p.login_parameters = query.value(11).toString();
			p.login_type = (ForumParser::ForumLoginType) query.value(12).toInt();
			p.charset = query.value(13).toString();
			p.thread_list_page_start = query.value(14).toInt();
			p.thread_list_page_increment = query.value(15).toInt();
			p.view_thread_page_start = query.value(16).toInt();
			p.view_thread_page_increment = query.value(17).toInt();
			p.forum_software = query.value(18).toString();
			p.view_message_path = query.value(19).toString();
			p.parser_type = query.value(20).toInt();
			p.posting_path = query.value(21).toString();
			p.posting_subject = query.value(22).toString();
			p.posting_message = query.value(23).toString();
			p.posting_parameters = query.value(24).toString();
			p.posting_hints = query.value(25).toString();
			p.reply_path = query.value(26).toString();
			p.reply_subject = query.value(27).toString();
			p.reply_message = query.value(28).toString();
			p.reply_parameters = query.value(29).toString();
		}
	} else {
		qDebug() << "Unable to list parsers!!";
	}
	return p;
}

QList<ForumParser> ParserDatabase::listParsers() {
	QSqlQuery query;
	QList<ForumParser> parsers;
	// @todo single select would be faster but wth
	if (query.exec("SELECT id FROM parsers")) {
		while (query.next()) {
			int id = query.value(0).toInt();
			parsers.append(getParser(id));
		}
	} else {
		qDebug() << "Unable to list parsers!!";
	}
	return parsers;
}
