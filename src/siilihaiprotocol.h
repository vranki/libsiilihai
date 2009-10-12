/*
 * siilihaiprotocol.h
 *
 *  Created on: Sep 18, 2009
 *      Author: vranki
 */

#ifndef SIILIHAIPROTOCOL_H_
#define SIILIHAIPROTOCOL_H_
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkCookieJar>
#include <QDomDocument>
#include <QDebug>
#include <QHash>
#include <QList>
#include "forumparser.h"
#include "httppost.h"

#define CLIENT_VERSION "0.6"

class SiilihaiProtocol : public QObject {
	Q_OBJECT
public:
	SiilihaiProtocol(QObject *parent=0);
	virtual ~SiilihaiProtocol();
	void login(QString user, QString pass);
	void setBaseURL(QString bu);
	void listParsers();
	void getParser(const int id);
	void subscribeForum(const int id, const int latest_threads, const int latest_messages, bool unsubscribe=false);
	void saveParser(const ForumParser parser);
public slots:
	void replyLogin(QNetworkReply *reply);
	void replyListParsers(QNetworkReply *reply);
	void replyGetParser(QNetworkReply *reply);
	void replySaveParser(QNetworkReply *reply);
	void replySubscribeForum(QNetworkReply *reply);
signals:
	void loginFinished(bool success, QString motd);
	void listParsersFinished(QList <ForumParser> parsers);
	void subscribeForumFinished(bool success);
	void getParserFinished(ForumParser parser);
	void saveParserFinished(int newId, QString message);
private:
	QString clientKey;
	QNetworkAccessManager nam;
	QString baseUrl;
	QByteArray loginData, listParsersData, saveParserData, getParserData, subscribeForumData;
	QUrl listParsersUrl, loginUrl, getParserUrl, saveParserUrl, subscribeForumUrl;
};

#endif /* SIILIHAIPROTOCOL_H_ */
