/*
 * forumsession.h
 *
 *  Created on: Sep 28, 2009
 *      Author: vranki
 */

#ifndef FORUMSESSION_H_
#define FORUMSESSION_H_
#include <QObject>
#include <QString>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkCookieJar>
#include <QDebug>

#include "forumparser.h"
#include "forumsubscription.h"
#include "httppost.h"
#include "patternmatcher.h"
#include "forumgroup.h"
#include "forumthread.h"
#include "forummessage.h"

class ForumSession : public QObject {
	Q_OBJECT
public:
	enum ForumSessionOperation { FSONoOp, FSOListGroups, FSOUpdateThreads, FSOUpdateMessages };

	ForumSession(QObject *parent=0);
	virtual ~ForumSession();
	void initialize(ForumParser &fop, ForumSubscription &fos, PatternMatcher *matcher=0);
	void setParser(ForumParser &fop);
	void listGroups();
	void updateGroup(ForumGroup group);
	void updateThread(ForumThread thread);
	QString getMessageUrl(const ForumMessage &msg);

	void performListGroups(QString &html);
	void performListThreads(QString &html);

public slots:
	void listGroupsReply(QNetworkReply *reply);
	void listThreadsReply(QNetworkReply *reply);
	void listMessagesReply(QNetworkReply *reply);
	void fetchCookieReply(QNetworkReply *reply);
	void cancelOperation();

signals:
	void listGroupsFinished(QList<ForumGroup> groups);
	void listThreadsFinished(QList<ForumThread> threads, ForumGroup group);
	void listMessagesFinished(QList<ForumMessage> messages, ForumThread thread);
	void groupUpdated(QList<ForumThread> threads);
	void networkFailure(QString message);

private:
	void fetchCookie();
	void updateGroupPage();
	void updateThreadPage();
	QString convertCharset(const QByteArray &src);
	QString statusReport();
	PatternMatcher *pm;
	ForumParser fpar;
	ForumSubscription fsub;
	QNetworkAccessManager nam;
	QByteArray emptyData;
	bool cookieFetched;
	ForumSessionOperation operationInProgress;
	QNetworkCookieJar *cookieJar;
	int currentListPage;
	ForumGroup currentGroup;
	ForumThread currentThread;
	QList<ForumThread> threads;
	QList<ForumMessage> messages;
	QString currentMessagesUrl;
};

#endif /* FORUMSESSION_H_ */
