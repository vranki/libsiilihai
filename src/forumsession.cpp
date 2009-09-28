/*
 * forumsession.cpp
 *
 *  Created on: Sep 28, 2009
 *      Author: vranki
 */

#include "forumsession.h"

ForumSession::ForumSession(QObject *parent) : QObject(parent) {
}

ForumSession::~ForumSession() {
	// TODO Auto-generated destructor stub
}

void ForumSession::listGroupsReply(QNetworkReply *reply) {
//	QHash<QString, QString>
}

void ForumSession::listGroups() {
	QNetworkRequest req(QUrl(fpar.forum_url));
/*
	listParsersData = HttpPost::setPostParameters(&req, params);
	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(replyListParsers(QNetworkReply*)));
			*/
	nam.post(req, listGroupsData);
}

void ForumSession::initialize(ForumParser &fop, ForumSubscription &fos) {
	fsub = fos;
	fpar = fop;
}
