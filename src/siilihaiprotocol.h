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
#include <QDomDocument>
#include <QDebug>
#include <QHash>
#include <QList>
#include "forumparser.h"
#include "httppost.h"

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
public slots:
	void replyLogin(QNetworkReply *reply);
	void replyListParsers(QNetworkReply *reply);
	void replyGetParser(QNetworkReply *reply);
	void replySubscribeForum(QNetworkReply *reply);
signals:
	void loginFinished(bool success);
	void listParsersFinished(QList <ForumParser> parsers);
	void subscribeForumFinished(bool success);
	void getParserFinished(ForumParser parser);
private:
	QString clientKey;
	QNetworkAccessManager nam;
	QString baseUrl;
	QByteArray loginData, listParsersData, getParserData, subscribeForumData;
	QUrl listParsersUrl, loginUrl, getParserUrl, subscribeForumUrl;
};

#endif /* SIILIHAIPROTOCOL_H_ */
