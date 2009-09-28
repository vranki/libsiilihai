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
#include <QDebug>

#include "forumparser.h"
#include "forumsubscription.h"
#include "httppost.h"

class ForumSession : public QObject {
	Q_OBJECT
public:
	ForumSession(QObject *parent=0);
	virtual ~ForumSession();
	void initialize(ForumParser &fop, ForumSubscription &fos);
	void listGroups();
public slots:
	void listGroupsReply(QNetworkReply *reply);

signals:
	void listGroupsFinished(QHash<QString, QString> groups);

private:
	ForumParser fpar;
	ForumSubscription fsub;
	QNetworkAccessManager nam;
	QByteArray listGroupsData;
};

#endif /* FORUMSESSION_H_ */
