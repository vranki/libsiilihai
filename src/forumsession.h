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

class ForumSession : public QObject {
	Q_OBJECT
public:
	enum ForumSessionOperation { FSONoOp, FSOListGroups, FSOUpdateAll };

	ForumSession(QObject *parent=0);
	virtual ~ForumSession();
	void initialize(ForumParser &fop, ForumSubscription &fos);
	void listGroups();
public slots:
	void listGroupsReply(QNetworkReply *reply);
	void fetchCookieReply(QNetworkReply *reply);

signals:
	void listGroupsFinished(QList<ForumGroup> groups);

private:
	void fetchCookie();
	QString convertCharset(const QByteArray &src);

	ForumParser fpar;
	ForumSubscription fsub;
	QNetworkAccessManager nam;
	QByteArray emptyData;
	bool cookieFetched;
	ForumSessionOperation operationInProgress;
	QNetworkCookieJar *cookieJar;
};

#endif /* FORUMSESSION_H_ */
