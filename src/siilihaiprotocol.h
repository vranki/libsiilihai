#ifndef SIILIHAIPROTOCOL_H_
#define SIILIHAIPROTOCOL_H_
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkProxy>
#include <QNetworkCookieJar>
#include <QDomDocument>
#include <QDebug>
#include <QHash>
#include <QList>
#include "forumparser.h"
#include "forumrequest.h"
#include "forumsubscription.h"
#include "forumgroup.h"
#include "forumthread.h"
#include "forummessage.h"
#include "httppost.h"
#include "parserreport.h"

#define CLIENT_VERSION "development"

class SiilihaiProtocol: public QObject {
Q_OBJECT
public:
	SiilihaiProtocol(QObject *parent = 0);
	virtual ~SiilihaiProtocol();
	void login(QString user, QString pass);
	void registerUser(QString user, QString pass, QString email);
	void setBaseURL(QString &bu);
	QString baseURL();
	void listParsers();
	void listRequests();
	void listSubscriptions();
	void getParser(const int id);
        void subscribeForum(const ForumSubscription *fs, bool unsubscribe = false);
	void subscribeGroups(QList<ForumGroup*> &fgs);
	void sendThreadData(QList<ForumMessage*> &fms);
	void saveParser(const ForumParser &parser);
	void getSyncSummary();
	void getThreadData(ForumGroup *grp);
public slots:
	void sendParserReport(ParserReport pr);
	void replyLogin(QNetworkReply *reply);
	void replyListParsers(QNetworkReply *reply);
	void replyListRequests(QNetworkReply *reply);
	void replyGetParser(QNetworkReply *reply);
	void replySaveParser(QNetworkReply *reply);
	void replySubscribeForum(QNetworkReply *reply);
	void replyListSubscriptions(QNetworkReply *reply);
	void replySendParserReport(QNetworkReply *reply);
	void replySubscribeGroups(QNetworkReply *reply);
	void replySendThreadData(QNetworkReply *reply);
	void replyGetSyncSummary(QNetworkReply *reply);
	void replyGetThreadData(QNetworkReply *reply);

signals:
	void loginFinished(bool success, QString motd);
	void listParsersFinished(QList<ForumParser> parsers);
	void listRequestsFinished(QList<ForumRequest> requests);
	void subscribeForumFinished(bool success);
	void getParserFinished(ForumParser parser);
	void saveParserFinished(int newId, QString message);
	void listSubscriptionsFinished(QList<int> subscriptions);
	void sendParserReportFinished(bool success);
	void subscribeGroupsFinished(bool success);
	void sendThreadDataFinished(bool success);
	void serverGroupStatus(QList<ForumGroup> grps);
	void serverThreadData(ForumThread &thread);
	void serverMessageData(ForumMessage &message);
	void getThreadDataFinished(bool success);

private:
	QString clientKey;
	QNetworkAccessManager nam;
	QString baseUrl;
	QByteArray loginData, listParsersData, saveParserData, getParserData,
			subscribeForumData, listRequestsData, registerData, listSubscriptionsData,
			sendParserReportData, subscribeGroupsData, sendThreadDataData, getThreadDataData,
			syncSummaryData;
	QUrl listParsersUrl, loginUrl, getParserUrl, saveParserUrl,
			subscribeForumUrl, listRequestsUrl, registerUrl, listSubscriptionsUrl,
			sendParserReportUrl, subscribeGroupsUrl, sendThreadDataUrl, getThreadDataUrl, syncSummaryUrl;
	ForumGroup *threadSummaryGroup;
};

#endif /* SIILIHAIPROTOCOL_H_ */
