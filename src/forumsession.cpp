/*
 * forumsession.cpp
 *
 *  Created on: Sep 28, 2009
 *      Author: vranki
 */

#include "forumsession.h"

ForumSession::ForumSession(QObject *parent) :
	QObject(parent) {
	operationInProgress = FSONoOp;
	cookieJar = new QNetworkCookieJar();
	nam.setCookieJar(cookieJar);
}

ForumSession::~ForumSession() {
	// TODO Auto-generated destructor stub
}
QString ForumSession::convertCharset(const QByteArray &src) {
	if (fpar.charset == "" || fpar.charset == "utf-8") {
		return QString(src);
		// return (QString().fromUtf8(src.data()));
	}
	// @todo may not work!
	if (fpar.charset == "iso-8859-1" || fpar.charset == "iso-8859-15") {
		return (QString().fromLatin1(src.data()));
	}
	// @todo smarter
	qDebug() << "Unknown charset " << fpar.charset << " - assuming ASCII";
	return (QString().fromAscii(src.data()));

}

void ForumSession::listGroupsReply(QNetworkReply *reply) {
	QList<ForumGroup> groups;
	qDebug() << "RX listgroups: ";
	disconnect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(listGroupsReply(QNetworkReply*)));
	QString data = convertCharset(reply->readAll());
	qDebug() << "Data: " << data;
	static PatternMatcher pm;
	if (!pm.isPatternSet()) {
		pm.setPattern(fpar.group_list_pattern);
	}
	QList<QHash<QString, QString> > matches = pm.findMatches(data);
	for (int i = 0; i < matches.size(); i++) {
		ForumGroup fg;
		QHash<QString, QString> match = matches[i];
		qDebug() << "Match " << i;
		QHashIterator<QString, QString> hi(match);
		while (hi.hasNext()) {
			hi.next();
			qDebug() << "\t" << hi.key() << ": " << hi.value();
		}
		fg.id = match["%a"];
		fg.name = match["%b"];
		fg.lastchange = match["%c"];
		if (fg.id.length() > 0 && fg.name.length() > 0) {
			groups.append(fg);
		} else {
			qDebug() << "Incomplete group, not adding";
		}
	}
	emit(listGroupsFinished(groups));
	operationInProgress = FSONoOp;
}

void ForumSession::fetchCookieReply(QNetworkReply *reply) {
	qDebug() << "RX fetchcookie: ";
	cookieFetched = true;
	disconnect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(fetchCookieReply(QNetworkReply*)));
	QList<QNetworkCookie> cookies = cookieJar->cookiesForUrl(QUrl(
			fpar.forum_url));
	for (int i = 0; i < cookies.size(); i++) {
		qDebug() << "Got cookie:" << cookies[i].name();
	}
	switch (operationInProgress) {
	case FSOListGroups:
		listGroups();
	}
}

void ForumSession::listGroups() {
	if(operationInProgress != FSONoOp) {
		qDebug() << "Operation in progress!! Don't command me yet!";
	}
	operationInProgress = FSOListGroups;
	if (!cookieFetched) {
		fetchCookie();
		return;
	}
	QNetworkRequest req(QUrl(fpar.forum_url));
	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(listGroupsReply(QNetworkReply*)));
	nam.post(req, emptyData);
}

void ForumSession::fetchCookie() {
	QNetworkRequest req(QUrl(fpar.forum_url));
	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(fetchCookieReply(QNetworkReply*)));
	nam.post(req, emptyData);
}

void ForumSession::initialize(ForumParser &fop, ForumSubscription &fos) {
	fsub = fos;
	fpar = fop;

	cookieFetched = false;
	operationInProgress = FSONoOp;
}
