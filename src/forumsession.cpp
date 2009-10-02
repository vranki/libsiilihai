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
		return QString().fromUtf8(src.data());
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
	// qDebug() << "Data: " << data;
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
		fg.parser = fpar.id;
		fg.id = match["%a"];
		fg.name = match["%b"];
		fg.lastchange = match["%c"];
		if (fg.isSane()) {
			groups.append(fg);
		} else {
			qDebug() << "Insane group, not adding";
			Q_ASSERT(false);
		}
	}
	operationInProgress = FSONoOp;
	emit(listGroupsFinished(groups));
}

void ForumSession::fetchCookieReply(QNetworkReply *reply) {
	qDebug() << "RX cookies:";
	cookieFetched = true;
	disconnect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(fetchCookieReply(QNetworkReply*)));
	QList<QNetworkCookie> cookies = cookieJar->cookiesForUrl(QUrl(
			fpar.forum_url));
	for (int i = 0; i < cookies.size(); i++) {
		qDebug() << "\t" << cookies[i].name();
	}

	switch (operationInProgress) {
	case FSOListGroups:
		listGroups();
		break;
	case FSOUpdateThreads:
		updateGroup(currentGroup);
		break;
	default:
		Q_ASSERT(false);
	}
}

void ForumSession::listGroups() {
	if (operationInProgress != FSONoOp && operationInProgress != FSOListGroups) {
		qDebug()
				<< "FS::listGroups(): Operation in progress!! Don't command me yet!";
		Q_ASSERT(false);
		return;
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

void ForumSession::updateGroupPage() {
	Q_ASSERT(operationInProgress==FSOUpdateThreads);

	QString urlString = fpar.thread_list_path;
	urlString = urlString.replace("%g", currentGroup.id);
	if (fpar.supportsThreadPages()) {
		urlString = urlString.replace("%p", QString().number(
				currentListPage));
	}
	urlString = fpar.forumUrlWithoutEnd() + urlString;
	qDebug() << "Fetching URL " << urlString;
	QNetworkRequest req;
	req.setUrl(QUrl(urlString));

	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(listThreadsReply(QNetworkReply*)));

	nam.post(req, emptyData);
}

void ForumSession::updateThreadPage() {
	Q_ASSERT(operationInProgress==FSOUpdateMessages);

	QString urlString = fpar.view_thread_path;
	urlString = urlString.replace("%g", currentGroup.id);
	urlString = urlString.replace("%t", currentThread.id);
	currentListPage = fpar.view_thread_page_start;
	if (currentListPage >= 0) {
		urlString = urlString.replace("%p", QString().number(
				currentListPage));
	}
	urlString = fpar.forumUrlWithoutEnd() + urlString;
	qDebug() << "Fetching URL " << urlString;
	QNetworkRequest req;
	req.setUrl(QUrl(urlString));

	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(listMessagesReply(QNetworkReply*)));

	nam.post(req, emptyData);
}

void ForumSession::updateGroup(ForumGroup group) {
	qDebug() << "ForumSession::UpdateGroup: " << group.toString();

	if (operationInProgress != FSONoOp) {
		qDebug() << "Operation in progress!! Don't command me yet!";
		Q_ASSERT(false);
		return;
	}
	operationInProgress = FSOUpdateThreads;
	currentGroup = group;
	if (!cookieFetched) {
		fetchCookie();
		return;
	}
	currentListPage = fpar.thread_list_page_start;
	updateGroupPage();
}

void ForumSession::updateThread(ForumThread thread) {
	qDebug() << "ForumSession::UpdateThread: " << thread.toString();

	if (operationInProgress != FSONoOp) {
		qDebug() << "Operation in progress!! Don't command me yet!";
		Q_ASSERT(false);
		return;
	}
	operationInProgress = FSOUpdateMessages;
	currentThread = thread;

	if (!cookieFetched) {
		fetchCookie();
		return;
	}
	updateThreadPage();
}

void ForumSession::listMessagesReply(QNetworkReply *reply) {
	QList<ForumMessage> newMessages;
	qDebug() << "RX listmessages: ";
	disconnect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(listMessagesReply(QNetworkReply*)));
	QString data = convertCharset(reply->readAll());
	qDebug() << "Pattern: " << fpar.message_list_pattern;
	// qDebug() << "Data: " << data;
	static PatternMatcher pm;
	if (!pm.isPatternSet()) {
		pm.setPattern(fpar.message_list_pattern);
	}
	QList<QHash<QString, QString> > matches = pm.findMatches(data);
	qDebug() << "ListMessages Found " << matches.size() << " matches";
	for (int i = 0; i < matches.size(); i++) {
		ForumMessage fm;
		QHash<QString, QString> match = matches[i];
		 qDebug() << "Match " << i;
		 QHashIterator<QString, QString> hi(match);
		 while (hi.hasNext()) {
		 hi.next();
		 qDebug() << "\t" << hi.key() << ": " << hi.value();
		 }
		fm.forumid = fpar.id;
		fm.groupid = currentThread.groupid;
		fm.threadid = currentThread.id;
		fm.read = false;
		fm.id = match["%a"];
		fm.subject = match["%b"];
		fm.body = match["%c"];
		fm.author = match["%d"];
		fm.lastchange = match["%e"];
		if (fm.isSane()) {
			newMessages.append(fm);
		} else {
			qDebug() << "Incomplete message, not adding";
			Q_ASSERT(false);
		}
	}
	// See if the new threads contains already unknown threads
	bool newMessagesFound = false;
	for (int i = 0; i < newMessages.size(); i++) {
		bool messageFound = false;
		for (int j = 0; j < messages.size(); j++) {
			if (newMessages[i].id == messages[j].id) {
				messageFound = true;
			}
		}
		if (!messageFound) {
			newMessagesFound = true;
			newMessages[i].ordernum = messages.size();
			messages.append(newMessages[i]);
		}
	}
	if (newMessagesFound) {
		if (fpar.supportsMessaegePages()) {
			// Continue to next page
			currentListPage += fpar.view_thread_page_increment;
			qDebug() << "New messages were found - continuing to next page "
					<< currentListPage;
			updateThreadPage();
		} else {
			qDebug() << "Forum doesn't support multipage - NOT continuing to next page.";
			operationInProgress = FSONoOp;
			emit
			(listMessagesFinished(messages, currentThread));
		}
	} else {
		qDebug() << "NOT continuing to next page, no new messages found on this one.";
		operationInProgress = FSONoOp;
		emit
		(listMessagesFinished(messages, currentThread));
	}
	if(operationInProgress == FSONoOp) {
		messages.clear();
	}
}

void ForumSession::listThreadsReply(QNetworkReply *reply) {
	QList<ForumThread> newThreads;
	qDebug() << "RX listthreads: ";
	disconnect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(listThreadsReply(QNetworkReply*)));
	QString data = convertCharset(reply->readAll());
	qDebug() << "Pattern: " << fpar.thread_list_pattern;
	// qDebug() << "Data: " << data;
	static PatternMatcher pm;
	if (!pm.isPatternSet()) {
		pm.setPattern(fpar.thread_list_pattern);
	}
	QList<QHash<QString, QString> > matches = pm.findMatches(data);
	qDebug() << "ListThreads Found " << matches.size() << " matches";
	for (int i = 0; i < matches.size(); i++) {
		ForumThread ft;
		QHash<QString, QString> match = matches[i];
		/*
		 qDebug() << "Match " << i;
		 QHashIterator<QString, QString> hi(match);
		 while (hi.hasNext()) {
		 hi.next();
		 qDebug() << "\t" << hi.key() << ": " << hi.value();
		 }
		 */
		ft.forumid = fpar.id;
		ft.groupid = currentGroup.id;
		ft.id = match["%a"];
		ft.name = match["%b"];
		ft.lastchange = match["%c"];
		if (ft.isSane()) {
			newThreads.append(ft);
		} else {
			qDebug() << "Incomplete thread, not adding";
			Q_ASSERT(false);
		}
	}
	// See if the new threads contains already unknown threads
	bool newThreadsFound = false;
	for (int i = 0; i < newThreads.size(); i++) {
		bool threadFound = false;
		for (int j = 0; j < threads.size(); j++) {
			if (newThreads[i].id == threads[j].id) {
				threadFound = true;
			}
		}
		if (!threadFound) {
			newThreadsFound = true;
			newThreads[i].ordernum = threads.size();
			threads.append(newThreads[i]);
		}
	}
	if (newThreadsFound) {
		if (fpar.supportsThreadPages()) {
			// Continue to next page
			currentListPage += fpar.thread_list_page_increment;
			qDebug() << "New threads were found - continuing to next page "
					<< currentListPage;
			updateGroupPage();
		} else {
			qDebug() << "Forum doesn't support multipage - NOT continuing to next page.";
			operationInProgress = FSONoOp;
			emit
			(listThreadsFinished(threads, currentGroup));
		}
	} else {
		qDebug() << "NOT continuing to next page.";
		operationInProgress = FSONoOp;
		emit
		(listThreadsFinished(threads, currentGroup));
	}
	if(operationInProgress == FSONoOp) {
		threads.clear();
	}
}
