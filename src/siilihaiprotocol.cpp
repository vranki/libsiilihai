/*
 * siilihaiprotocol.cpp
 *
 *  Created on: Sep 18, 2009
 *      Author: vranki
 */

#include "siilihaiprotocol.h"

SiilihaiProtocol::SiilihaiProtocol(QObject *parent) :
	QObject(parent) {
	// TODO Auto-generated constructor stub

}

SiilihaiProtocol::~SiilihaiProtocol() {
	// TODO Auto-generated destructor stub
}

void SiilihaiProtocol::listParsers() {
	QNetworkRequest req(listParsersUrl);
	QHash<QString, QString> params;
	if (!clientKey.isNull()) {
		params.insert("client_key", clientKey);
	}
	listParsersData = HttpPost::setPostParameters(&req, params);
	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(replyListParsers(QNetworkReply*)));
	nam.post(req, listParsersData);
}

void SiilihaiProtocol::login(QString user, QString pass) {
	QNetworkRequest req(loginUrl);
	QHash<QString, QString> params;
	params.insert("username", user);
	params.insert("password", pass);
	params.insert("action", "login");
	loginData = HttpPost::setPostParameters(&req, params);
	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(replyLogin(QNetworkReply*)));
	nam.post(req, loginData);
}

void SiilihaiProtocol::getParser(const int id) {
	QNetworkRequest req(getParserUrl);
	QHash<QString, QString> params;
	params.insert("id", QString().number(id));
	if (!clientKey.isNull()) {
		params.insert("client_key", clientKey);
	}

	getParserData = HttpPost::setPostParameters(&req, params);
	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(replyGetParser(QNetworkReply*)));
	nam.post(req, getParserData);
}

void SiilihaiProtocol::setBaseURL(QString bu) {
	baseUrl = bu;
	listParsersUrl = QUrl(baseUrl + "api/forumlist.xml");
	loginUrl = QUrl(baseUrl + "api/login.xml");
	getParserUrl = QUrl(baseUrl + "api/getparser.xml");
	subscribeForumUrl = QUrl(baseUrl + "api/subscribeforum.xml");
}

void SiilihaiProtocol::subscribeForum(const int id, const int latest_threads,
		const int latest_messages, bool unsubscribe) {
	QNetworkRequest req(subscribeForumUrl);
	QHash<QString, QString> params;
	params.insert("parser_id", QString().number(id));
	if (unsubscribe) {
		params.insert("unsubscribe", "yes");
	} else {
		params.insert("latest_threads", QString().number(latest_threads));
		params.insert("latest_messages", QString().number(latest_messages));
	}
	if (!clientKey.isNull()) {
		params.insert("client_key", clientKey);
	}
	subscribeForumData = HttpPost::setPostParameters(&req, params);
	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(replySubscribeForum(QNetworkReply*)));
	nam.post(req, subscribeForumData);
}

void SiilihaiProtocol::replySubscribeForum(QNetworkReply *reply) {
	bool success = reply->error() == QNetworkReply::NoError;
	nam.disconnect(SIGNAL(finished(QNetworkReply*)));
	emit(subscribeForumFinished(success));
	reply->deleteLater();
}


void SiilihaiProtocol::replyListParsers(QNetworkReply *reply) {
	QString docs = QString().fromUtf8(reply->readAll());
	qDebug() << docs;
	QString ck = QString::null;
	QList<ForumParser> parsers;
	if (reply->error() == QNetworkReply::NoError) {
		QDomDocument doc;
		doc.setContent(docs);
		QDomNode n = doc.firstChild().firstChild();
		while (!n.isNull()) {
			ForumParser parser;
			parser.id = QString(n.firstChildElement("id").text()).toInt();
			parser.forum_url = n.firstChildElement("forum_url").text();
			parser.parser_name = n.firstChildElement("name").text();
			parser.parser_status
					= QString(n.firstChildElement("status").text()).toInt();
			parser.parser_type
					= QString(n.firstChildElement("type").text()).toInt();
			parsers.append(parser);
			n = n.nextSibling();
		}
		ck = doc.documentElement().text();
	} else {
		qDebug() << "error!";
	}
	nam.disconnect(SIGNAL(finished(QNetworkReply*)));
	emit
	(listParsersFinished(parsers));
	reply->deleteLater();
}

void SiilihaiProtocol::replyLogin(QNetworkReply *reply) {
	QString docs = QString().fromUtf8(reply->readAll());
	qDebug() << docs;
	QString ck = QString::null;
	if (reply->error() == QNetworkReply::NoError) {
		QDomDocument doc;
		doc.setContent(docs);
		ck = doc.documentElement().text();
	} else {
		qDebug() << "error!";
	}
	qDebug() << "got ck" << ck;
	clientKey = ck;
	nam.disconnect(SIGNAL(finished(QNetworkReply*)));
	emit
	(loginFinished(!ck.isNull()));
	reply->deleteLater();
}

void SiilihaiProtocol::replyGetParser(QNetworkReply *reply) {
	QString docs = QString().fromUtf8(reply->readAll());
	ForumParser parser;
	parser.id = -1;
	qDebug() << docs;
	if (reply->error() == QNetworkReply::NoError) {
		QDomDocument doc;
		doc.setContent(docs);
		QDomElement re = doc.firstChild().toElement();
		parser.id = re.firstChildElement("id").text().toInt();
		parser.parser_name = re.firstChildElement("parser_name").text();
		parser.forum_url = re.firstChildElement("forum_url").text();
		parser.parser_status = re.firstChildElement("status").text().toInt();
		parser.thread_list_path
				= re.firstChildElement("thread_list_path").text();
		parser.view_thread_path
				= re.firstChildElement("view_thread_path").text();
		parser.login_path = re.firstChildElement("login_path").text();
		parser.date_format = re.firstChildElement("date_format").text().toInt();
		parser.group_list_pattern
				= re.firstChildElement("group_list_pattern").text();
		parser.thread_list_pattern
				= re.firstChildElement("thread_list_pattern").text();
		parser.message_list_pattern = re.firstChildElement(
				"message_list_pattern").text();
		parser.verify_login_pattern = re.firstChildElement(
				"verify_login_pattern").text();
		parser.login_parameters
				= re.firstChildElement("login_parameters").text();
		parser.login_type = re.firstChildElement("login_type").text().toInt();
		parser.charset = re.firstChildElement("charset").text();
		parser.thread_list_page_start = re.firstChildElement(
				"thread_list_page_start").text().toInt();
		parser.thread_list_page_increment = re.firstChildElement(
				"thread_list_page_increment").text().toInt();
		parser.view_thread_page_start = re.firstChildElement(
				"view_thread_page_start").text().toInt();
		parser.view_thread_page_increment = re.firstChildElement(
				"view_thread_page_increment").text().toInt();

		parser.forum_software = re.firstChildElement("forum_software").text();
		parser.view_message_path
				= re.firstChildElement("view_message_path").text();
		parser.parser_type = re.firstChildElement("parser_type").text().toInt();
		parser.posting_path = re.firstChildElement("posting_path").text();
		parser.posting_subject = re.firstChildElement("posting_subject").text();
		parser.posting_message = re.firstChildElement("posting_message").text();
		parser.posting_parameters
				= re.firstChildElement("posting_parameters").text();
		parser.posting_hints = re.firstChildElement("posting_hints").text();
	} else {
		qDebug() << "error!";
	}
	nam.disconnect(SIGNAL(finished(QNetworkReply*)));
	emit
	(getParserFinished(parser));
	reply->deleteLater();
}
