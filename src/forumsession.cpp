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
	qDebug() << "FS::listGroupsReply:";
	qDebug() << statusReport();

	disconnect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(listGroupsReply(QNetworkReply*)));
	QString data = convertCharset(reply->readAll());

	if (reply->error() != QNetworkReply::NoError) {
		emit(networkFailure(reply->errorString()));
		cancelOperation();
		return;
	}

	performListGroups(data);
}

void ForumSession::performListGroups(QString &html) {
	QList<ForumGroup> groups;
	pm->setPattern(fpar.group_list_pattern);
	qDebug() << "PLG for " << html.length();
	QList<QHash<QString, QString> > matches = pm->findMatches(html);
	for (int i = 0; i < matches.size(); i++) {
		ForumGroup fg;
		QHash<QString, QString> match = matches[i];

		fg.parser = fpar.id;
		fg.id = match["%a"];
		fg.name = match["%b"];
		fg.lastchange = match["%c"];
		if (fg.isSane()) {
			groups.append(fg);
		} else {
			qDebug() << "Insane group, not adding";
			// Q_ASSERT(false);
		}
	}
	operationInProgress = FSONoOp;
	emit(listGroupsFinished(groups));
}

void ForumSession::fetchCookieReply(QNetworkReply *reply) {
	qDebug() << "RX cookies:";
	qDebug() << statusReport();

	cookieFetched = true;
	disconnect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(fetchCookieReply(QNetworkReply*)));
	if (reply->error() != QNetworkReply::NoError) {
		emit(networkFailure(reply->errorString()));
		cancelOperation();
		return;
	}
	if (operationInProgress == FSONoOp)
		return;
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
		listThreads(currentGroup);
		break;
	case FSOUpdateMessages:
		listMessages(currentThread);
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
	if (operationInProgress == FSONoOp)
		return;
	QNetworkRequest req(QUrl(fpar.forum_url));
	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(fetchCookieReply(QNetworkReply*)));
	nam.post(req, emptyData);
}

void ForumSession::initialize(ForumParser &fop, ForumSubscription &fos,
		PatternMatcher *matcher) {
	fsub = fos;
	fpar = fop;

	cookieFetched = false;
	operationInProgress = FSONoOp;
	pm = matcher;
	if (!pm)
		pm = new PatternMatcher(this);
}

void ForumSession::updateGroupPage() {
	Q_ASSERT(operationInProgress==FSOUpdateThreads);
	if (operationInProgress != FSOUpdateThreads)
		return;

	QString urlString = getThreadListUrl(currentGroup, currentListPage);
	qDebug() << "Fetching URL " << urlString;
	QNetworkRequest req;
	req.setUrl(QUrl(urlString));

	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(listThreadsReply(QNetworkReply*)));

	nam.post(req, emptyData);
}

void ForumSession::updateThreadPage() {
	Q_ASSERT(operationInProgress==FSOUpdateMessages);
	if (operationInProgress != FSOUpdateMessages)
		return;

	QString urlString = getMessageListUrl(currentThread, currentListPage);
	qDebug() << "Fetching URL " << urlString;
	currentMessagesUrl = urlString;

	QNetworkRequest req;
	req.setUrl(QUrl(urlString));

	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(listMessagesReply(QNetworkReply*)));

	nam.post(req, emptyData);
}

void ForumSession::listThreads(ForumGroup group) {
	qDebug() << "ForumSession::UpdateGroup: " << group.toString();

	if (operationInProgress != FSONoOp && operationInProgress
			!= FSOUpdateThreads) {
		qDebug() << "Operation in progress!! Don't command me yet!";
		Q_ASSERT(false);
		return;
	}
	operationInProgress = FSOUpdateThreads;
	currentGroup = group;
	threads.clear();
	if (!cookieFetched) {
		fetchCookie();
		return;
	}
	currentListPage = fpar.thread_list_page_start;
	updateGroupPage();
}

void ForumSession::listMessages(ForumThread thread) {
	qDebug() << "ForumSession::UpdateThread: " << thread.toString();

	if (operationInProgress != FSONoOp && operationInProgress != FSOUpdateMessages) {
		qDebug() << "Operation in progress!! Don't command me yet!";
		Q_ASSERT(false);
		return;
	}
	operationInProgress = FSOUpdateMessages;
	currentThread = thread;
	messages.clear();

	if (!cookieFetched) {
		fetchCookie();
		return;
	}
	updateThreadPage();
}

void ForumSession::listMessagesReply(QNetworkReply *reply) {
	qDebug() << "RX listmessages: ";
	qDebug() << statusReport();
	disconnect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(listMessagesReply(QNetworkReply*)));
	if (reply->error() != QNetworkReply::NoError) {
		emit(networkFailure(reply->errorString()));
		cancelOperation();
		return;
	}
	QString data = convertCharset(reply->readAll());
	performListMessages(data);
}

void ForumSession::performListMessages(QString &html) {
	QList<ForumMessage> newMessages;
	qDebug() << "Pattern: " << fpar.message_list_pattern;
	// qDebug() << "Data: " << data;
	operationInProgress = FSOUpdateMessages;
	pm->setPattern(fpar.message_list_pattern);
	QList<QHash<QString, QString> > matches = pm->findMatches(html);
	qDebug() << "ListMessages Found " << matches.size() << " matches";
	for (int i = 0; i < matches.size(); i++) {
		ForumMessage fm;
		QHash<QString, QString> match = matches[i];
		fm.forumid = fpar.id;
		fm.groupid = currentThread.groupid;
		fm.threadid = currentThread.id;
		fm.read = false;
		fm.id = match["%a"];
		fm.subject = match["%b"];
		fm.body = match["%c"];
		fm.author = match["%d"];
		fm.lastchange = match["%e"];
		if (fpar.supportsMessageUrl()) {
			fm.url = getMessageUrl(fm);
		} else {
			fm.url = currentMessagesUrl;
		}
		if (fm.isSane()) {
			newMessages.append(fm);
		} else {
			qDebug() << "Incomplete message, not adding";
			// Q_ASSERT(false);
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
			if (messages.size() < fsub.latest_messages) {
				messages.append(newMessages[i]);
			} else {
				qDebug()
						<< "Number of messages exceeding maximum latest messages limit - not adding.";
				newMessagesFound = false;
			}
		}
	}
	if (newMessagesFound) {
		if (fpar.supportsMessagePages()) {
			// Continue to next page
			currentListPage += fpar.view_thread_page_increment;
			qDebug() << "New messages were found - continuing to next page "
					<< currentListPage;
			updateThreadPage();
		} else {
			qDebug()
					<< "Forum doesn't support multipage - NOT continuing to next page.";
			operationInProgress = FSONoOp;
			emit
			(listMessagesFinished(messages, currentThread));
			messages.clear();
		}
	} else {
		qDebug()
				<< "NOT continuing to next page, no new messages found on this one.";
		operationInProgress = FSONoOp;
		emit
		(listMessagesFinished(messages, currentThread));
		messages.clear();
	}
	if (operationInProgress == FSONoOp) {
		qDebug() << "clearing messages, size " << messages.size();
		messages.clear();
		currentThread.id = -1;
	}
}

QString ForumSession::statusReport() {
	QString op;
	if (operationInProgress == FSONoOp)
		op = "NoOp";
	if (operationInProgress == FSOListGroups)
		op = "ListGroups";
	if (operationInProgress == FSOUpdateThreads)
		op = "UpdateThreads";
	if (operationInProgress == FSOUpdateMessages)
		op = "UpdateMessages";

	return "Operation: " + op + " in " + fpar.toString() + "\n" + "Threads: "
			+ QString().number(threads.size()) + "\n" + "Messages: "
			+ QString().number(messages.size()) + "\n" + "Page: "
			+ QString().number(currentListPage) + "\n" + "Group: "
			+ currentGroup.toString() + "\n" + "Thread: "
			+ currentThread.toString() + "\n";
}

void ForumSession::listThreadsReply(QNetworkReply *reply) {
	qDebug() << "RX listthreads in group " << currentGroup.toString();
	qDebug() << statusReport();
	disconnect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(listThreadsReply(QNetworkReply*)));
	if (reply->error() != QNetworkReply::NoError) {
		emit(networkFailure(reply->errorString()));
		cancelOperation();
		return;
	}
	QString data = convertCharset(reply->readAll());
	performListThreads(data);
}

void ForumSession::performListThreads(QString &html) {
	QList<ForumThread> newThreads;
	operationInProgress = FSOUpdateThreads;
	// qDebug() << "Pattern: " << fpar.thread_list_pattern;
	// qDebug() << "Data: " << data;
	pm->setPattern(fpar.thread_list_pattern);
	QList<QHash<QString, QString> > matches = pm->findMatches(html);
	qDebug() << "ListThreads Found " << matches.size() << " matches";
	for (int i = 0; i < matches.size(); i++) {
		ForumThread ft;
		QHash<QString, QString> match = matches[i];
		ft.forumid = fpar.id;
		ft.groupid = currentGroup.id;
		ft.id = match["%a"];
		ft.name = match["%b"];
		ft.lastchange = match["%c"];
		if (ft.isSane()) {
			newThreads.append(ft);
		} else {
			qDebug() << "Incomplete thread, not adding";
			// Q_ASSERT(false);
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
			if (threads.size() < fsub.latest_threads) {
				threads.append(newThreads[i]);
			} else {
				qDebug()
						<< "Number of threads exceeding maximum latest threads limit - not adding "
						<< newThreads[i].toString();
				newThreadsFound = false;
			}
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
			qDebug()
					<< "Forum doesn't support multipage - NOT continuing to next page.";
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
	if (operationInProgress == FSONoOp) {
		threads.clear();
		currentThread.id = -1;
	}
}

void ForumSession::cancelOperation() {
	disconnect(&nam, SIGNAL(finished(QNetworkReply*)));
	operationInProgress = FSONoOp;
	cookieFetched = false;
	currentListPage = -1;
	threads.clear();
	messages.clear();
	currentGroup.id = -1;
	currentThread.id = -1;
}

QString ForumSession::getMessageUrl(const ForumMessage &msg) {
	QUrl url = QUrl();

	QString urlString = fpar.view_message_path;
	urlString = urlString.replace("%g", currentGroup.id);
	urlString = urlString.replace("%t", currentThread.id);
	urlString = urlString.replace("%m", msg.id);
	urlString = fpar.forumUrlWithoutEnd() + urlString;
	return urlString;
}

void ForumSession::setParser(ForumParser &fop) {
	fpar = fop;
}

QString ForumSession::getThreadListUrl(const ForumGroup &grp, int page) {
	QString urlString = fpar.thread_list_path;
	urlString = urlString.replace("%g", grp.id);
	if (fpar.supportsThreadPages()) {
		if (page < 0)
			page = fpar.thread_list_page_start;
		urlString = urlString.replace("%p", QString().number(page));
	}
	urlString = fpar.forumUrlWithoutEnd() + urlString;
	return urlString;
}

QString ForumSession::getMessageListUrl(const ForumThread &thread, int page) {
	QString urlString = fpar.view_thread_path;
	urlString = urlString.replace("%g", thread.groupid);
	urlString = urlString.replace("%t", thread.id);
	if (fpar.supportsMessagePages()) {
		//	currentListPage = fpar.view_thread_page_start;
		if (page < 0)
			page = fpar.view_thread_page_start;
		urlString = urlString.replace("%p", QString().number(page));

	}
	urlString = fpar.forumUrlWithoutEnd() + urlString;
	return urlString;
}
