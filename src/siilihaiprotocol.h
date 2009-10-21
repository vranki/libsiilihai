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
#include "forumrequest.h"
#include "httppost.h"

#define CLIENT_VERSION "0.6"

class SiilihaiProtocol: public QObject {
Q_OBJECT
public:
	SiilihaiProtocol(QObject *parent = 0);
	virtual ~SiilihaiProtocol();
	void login(QString user, QString pass);
	void registerUser(QString user, QString pass, QString email);
	void setBaseURL(QString bu);
	void listParsers();
	void listRequests();
	void getParser(const int id);
	void subscribeForum(const int id, const int latest_threads,
			const int latest_messages, bool unsubscribe = false);
	void saveParser(const ForumParser parser);
public slots:
// void replyRegister(QNetworkReply *reply);
	void replyLogin(QNetworkReply *reply);
	void replyListParsers(QNetworkReply *reply);
	void replyListRequests(QNetworkReply *reply);
	void replyGetParser(QNetworkReply *reply);
	void replySaveParser(QNetworkReply *reply);
	void replySubscribeForum(QNetworkReply *reply);
signals:
	void loginFinished(bool success, QString motd);
	void listParsersFinished(QList<ForumParser> parsers);
	void listRequestsFinished(QList<ForumRequest> requests);
	void subscribeForumFinished(bool success);
	void getParserFinished(ForumParser parser);
	void saveParserFinished(int newId, QString message);
private:
	QString clientKey;
	QNetworkAccessManager nam;
	QString baseUrl;
	QByteArray loginData, listParsersData, saveParserData, getParserData,
			subscribeForumData, listRequestsData, registerData;
	QUrl listParsersUrl, loginUrl, getParserUrl, saveParserUrl,
			subscribeForumUrl, listRequestsUrl, registerUrl;
};

#endif /* SIILIHAIPROTOCOL_H_ */
