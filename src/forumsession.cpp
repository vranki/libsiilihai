#include "forumsession.h"

ForumSession::ForumSession(QObject *parent) :
	QObject(parent) {
	operationInProgress = FSONoOp;
	nam = 0;
	cookieJar = 0;
	currentListPage = 0;
	pm = 0;
	loggedIn = false;
	cookieFetched = false;
	clearAuthentications();
}

ForumSession::~ForumSession() {
}

QString ForumSession::convertCharset(const QByteArray &src) {
	QString converted;
	// I don't like this.. Support needed for more!
	if (fpar.charset == "" || fpar.charset == "utf-8") {
		converted = QString().fromUtf8(src.data());
	} else if (fpar.charset == "iso-8859-1" || fpar.charset == "iso-8859-15") {
		converted = (QString().fromLatin1(src.data()));
	} else {
		qDebug() << "Unknown charset " << fpar.charset << " - assuming ASCII";
		converted = (QString().fromAscii(src.data()));
	}
	// Remove silly newlines
	converted.remove(QChar(13));
	return converted;
}

void ForumSession::listGroupsReply(QNetworkReply *reply) {
	qDebug() << Q_FUNC_INFO;
	qDebug() << statusReport();

	disconnect(nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(listGroupsReply(QNetworkReply*)));
	QString data = convertCharset(reply->readAll());
	qDebug() << "RX " << data.size() << " chars";
	if (reply->error() != QNetworkReply::NoError) {
		emit(networkFailure(reply->errorString()));
		cancelOperation();
		return;
	}

	performListGroups(data);
}

void ForumSession::loginReply(QNetworkReply *reply) {
	qDebug() << Q_FUNC_INFO;
	qDebug() << statusReport();

	disconnect(nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(loginReply(QNetworkReply*)));
	QString data = convertCharset(reply->readAll());

	if (reply->error() != QNetworkReply::NoError) {
		emit(networkFailure(reply->errorString()));
		loginFinished(false);
		cancelOperation();
		return;
	}

	performLogin(data);
}

void ForumSession::performListGroups(QString &html) {
	Q_ASSERT(pm);
	emit receivedHtml(html);
	QList<ForumGroup> groups;
	pm->setPattern(fpar.group_list_pattern);
	QList<QHash<QString, QString> > matches = pm->findMatches(html);
	for (int i = 0; i < matches.size(); i++) {
		ForumGroup fg(fsub);
		QHash<QString, QString> match = matches[i];
		fg.setId(match["%a"]);
		fg.setName(match["%b"]);
		fg.setLastchange(match["%c"]);
		groups.append(fg);
	}
	operationInProgress = FSONoOp;
	emit(listGroupsFinished(groups));
}

void ForumSession::performLogin(QString &html) {
	qDebug() << Q_FUNC_INFO;
	emit
	receivedHtml(html);
	bool success = html.contains(fpar.verify_login_pattern);
	emit loginFinished(success);
	loggedIn = success;
	if(loggedIn) {
		nextOperation();
	} else {
		cancelOperation();
		qDebug() << "Login failed - cancelling ops!";
	}
}

void ForumSession::fetchCookieReply(QNetworkReply *reply) {
	qDebug() << Q_FUNC_INFO;
	qDebug() << statusReport();

	cookieFetched = true;
	disconnect(nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(fetchCookieReply(QNetworkReply*)));
	if (reply->error() != QNetworkReply::NoError) {
		emit(networkFailure(reply->errorString()));
		cancelOperation();
		return;
	}
	if (operationInProgress == FSONoOp)
		return;
	/*
	QList<QNetworkCookie> cookies = cookieJar->cookiesForUrl(QUrl(
			fpar.forum_url));
	for (int i = 0; i < cookies.size(); i++) {
		qDebug() << "\t" << cookies[i].name();
	}
	*/
	nextOperation();
}

void ForumSession::listGroups() {
	qDebug() << Q_FUNC_INFO;
	if (operationInProgress != FSONoOp && operationInProgress != FSOListGroups) {
		qDebug()
				<< "FS::listGroups(): Operation in progress!! Don't command me yet!";
		Q_ASSERT(false);
		return;
	}
	operationInProgress = FSOListGroups;
	if(prepareForUse()) return;

	QNetworkRequest req(QUrl(fpar.forum_url));
	connect(nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(listGroupsReply(QNetworkReply*)));
	nam->post(req, emptyData);
}

void ForumSession::loginToForum() {
	qDebug() << Q_FUNC_INFO;

	if (fpar.login_type == ForumParser::LoginTypeNotSupported) {
		qDebug() << "Login not supproted!";
		emit
		loginFinished(false);
		return;
	}

	if (fsub->username().length() <= 0 || fsub->password().length() <= 0) {
		qDebug() << "Warning, no credentials supplied. Logging in should fail.";
	}

	qDebug() << "u/p: " << fsub->username() << "/" << fsub->password();
	QUrl loginUrl(getLoginUrl());
	if (fpar.login_type == ForumParser::LoginTypeHttpPost) {
		QNetworkRequest req;
		req.setUrl(loginUrl);
		QHash<QString, QString> params;
		QStringList loginParamPairs = fpar.login_parameters.split(",",
				QString::SkipEmptyParts);
		for (int i = 0; i < loginParamPairs.size(); ++i) {
			QString paramPair = loginParamPairs.at(i);
			paramPair = paramPair.replace("%u", fsub->username());
			paramPair = paramPair.replace("%p", fsub->password());
			qDebug() << "Param Pair: " << paramPair;

			if (paramPair.contains('=')) {
				QStringList singleParam = paramPair.split('=',
						QString::KeepEmptyParts);
				if (singleParam.size() == 2) {
					params.insert(singleParam.at(0), singleParam.at(1));
				} else {
					qDebug("hm, invalid login parameter pair!");
				}
			}
		}
		qDebug() << "Logging with " << params.size() << " params " << " to "
				<< getLoginUrl();
		loginData = HttpPost::setPostParameters(&req, params);

		connect(nam, SIGNAL(finished(QNetworkReply*)), this,
				SLOT(loginReply(QNetworkReply*)));
		nam->post(req, loginData);
	} else {
		qDebug("Sorry, http auth not yet implemented.");
	}
}

void ForumSession::fetchCookie() {
	qDebug() << Q_FUNC_INFO;

	if (operationInProgress == FSONoOp)
		return;
	QNetworkRequest req(QUrl(fpar.forum_url));
	connect(nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(fetchCookieReply(QNetworkReply*)));
	nam->post(req, emptyData);
}

void ForumSession::initialize(ForumParser &fop, ForumSubscription *fos,
		PatternMatcher *matcher) {
	fsub = fos;
	fpar = fop;

	cookieFetched = false;
	operationInProgress = FSONoOp;
	pm = matcher;
	if (!pm)
		pm = new PatternMatcher(this);
	nam->setProxy(QNetworkProxy::applicationProxy());
}

void ForumSession::updateGroupPage() {
	Q_ASSERT(operationInProgress==FSOUpdateThreads);
	if (operationInProgress != FSOUpdateThreads)
		return;

	QString urlString = getThreadListUrl(currentGroup, currentListPage);
	qDebug() << "Fetching URL " << urlString;
	QNetworkRequest req;
	req.setUrl(QUrl(urlString));

	connect(nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(listThreadsReply(QNetworkReply*)));

	nam->post(req, emptyData);
}

void ForumSession::updateThreadPage() {
	Q_ASSERT(operationInProgress==FSOUpdateMessages);
	if (operationInProgress != FSOUpdateMessages) {
		Q_ASSERT(false);
		return;
	}
	QString urlString = getMessageListUrl(currentThread, currentListPage);
	qDebug() << Q_FUNC_INFO << " Fetching URL " << urlString;
	currentMessagesUrl = urlString;

	QNetworkRequest req;
	req.setUrl(QUrl(urlString));

	connect(nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(listMessagesReply(QNetworkReply*)));

	nam->post(req, emptyData);
}

void ForumSession::listThreads(ForumGroup *group) {
	qDebug() << "ForumSession::UpdateGroup: " << group->toString();

	if (operationInProgress != FSONoOp && operationInProgress
			!= FSOUpdateThreads) {
		qDebug() << "Operation in progress!! Don't command me yet!";
		Q_ASSERT(false);
		return;
	}
	operationInProgress = FSOUpdateThreads;
	currentGroup = group;
	threads.clear();
	if(prepareForUse()) return;
	currentListPage = fpar.thread_list_page_start;
	updateGroupPage();
}

void ForumSession::listMessages(ForumThread *thread) {
	qDebug() << "ForumSession::UpdateThread: " << thread->toString();

	if (operationInProgress != FSONoOp && operationInProgress
			!= FSOUpdateMessages) {
		qDebug() << "Operation in progress!! Don't command me yet!";
		Q_ASSERT(false);
		return;
	}
	operationInProgress = FSOUpdateMessages;
	currentThread = thread;

	messages.clear();

	if(prepareForUse()) return;
	currentListPage = fpar.view_thread_page_start;
	updateThreadPage();
}

void ForumSession::listMessagesReply(QNetworkReply *reply) {
	qDebug() << Q_FUNC_INFO;
	qDebug() << statusReport();
	disconnect(nam, SIGNAL(finished(QNetworkReply*)), this,
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
	emit
	receivedHtml(html);
	operationInProgress = FSOUpdateMessages;
	pm->setPattern(fpar.message_list_pattern);
	QList<QHash<QString, QString> > matches = pm->findMatches(html);
	qDebug() << "ListMessages Found " << matches.size() << " matches";
	for (int i = 0; i < matches.size(); i++) {
		ForumMessage fm(currentThread);
		QHash<QString, QString> match = matches[i];
		fm.setRead(false);
		fm.setId(match["%a"]);
		fm.setSubject(match["%b"]);
		fm.setBody(match["%c"]);
		fm.setAuthor(match["%d"]);
		fm.setLastchange(match["%e"]);
		if (fpar.supportsMessageUrl()) {
			fm.setUrl(getMessageUrl(&fm));
		} else {
			fm.setUrl(currentMessagesUrl);
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
			if (newMessages[i].id() == messages[j].id()) {
				messageFound = true;
			}
		}
		if (!messageFound) {
			newMessagesFound = true;
			newMessages[i].setOrdernum(messages.size());
			if (messages.size() < fsub->latest_messages()) {
				messages.append(newMessages[i]);
			} else {
				qDebug()
						<< "Number of messages exceeding maximum latest messages limit - not adding.";
				newMessagesFound = false;
			}
		}
	}
	if (newMessagesFound) {
		if (fpar.view_thread_page_increment > 0) {
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
		currentThread = 0;
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
                        + QString().number(currentListPage)/* + "\n" + "Group: "
			+ currentGroup->toString() + "\n" + "Thread: "
                        + currentThread->toString() + "\n"*/;
}

void ForumSession::listThreadsReply(QNetworkReply *reply) {
	qDebug() << Q_FUNC_INFO << currentGroup->toString();
	qDebug() << statusReport();
	disconnect(nam, SIGNAL(finished(QNetworkReply*)), this,
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
	emit
	receivedHtml(html);
	operationInProgress = FSOUpdateThreads;
	pm->setPattern(fpar.thread_list_pattern);
	QList<QHash<QString, QString> > matches = pm->findMatches(html);
	qDebug() << "ListThreads Found " << matches.size() << " matches";
	for (int i = 0; i < matches.size(); i++) {
		ForumThread ft(currentGroup);
		QHash<QString, QString> match = matches[i];
		ft.setId(match["%a"]);
		ft.setName(match["%b"]);
		ft.setLastchange(match["%c"]);
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
			if (newThreads[i].id() == threads[j].id()) {
				threadFound = true;
			}
		}
		if (!threadFound) {
			newThreadsFound = true;
			newThreads[i].setOrdernum(threads.size());
			if (threads.size() < fsub->latest_threads()) {
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
		if (fpar.thread_list_page_increment > 0) {
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
		currentThread = 0;
	}
}

void ForumSession::cancelOperation() {
	qDebug() << Q_FUNC_INFO;
	if (nam)
		disconnect(nam, SIGNAL(finished(QNetworkReply*)));
	operationInProgress = FSONoOp;
	cookieFetched = false;
	currentListPage = -1;
	threads.clear();
	messages.clear();
	currentGroup = 0;
	currentThread = 0;
}

// @todo could be in forummessage?
QString ForumSession::getMessageUrl(const ForumMessage *msg) {
	QUrl url = QUrl();

	QString urlString = fpar.view_message_path;
	urlString = urlString.replace("%g", currentGroup->id());
	urlString = urlString.replace("%t", currentThread->id());
	urlString = urlString.replace("%m", msg->id());
	urlString = fpar.forumUrlWithoutEnd() + urlString;
	return urlString;
}

QString ForumSession::getLoginUrl() {
	return fpar.forumUrlWithoutEnd() + fpar.login_path;
}

void ForumSession::setParser(ForumParser &fop) {
	// Sanity check can't be done here as it would break parser maker
	fpar = fop;
}

QString ForumSession::getThreadListUrl(const ForumGroup *grp, int page) {
	QString urlString = fpar.thread_list_path;
	urlString = urlString.replace("%g", grp->id());
	if (fpar.supportsThreadPages()) {
		if (page < 0)
			page = fpar.thread_list_page_start;
		urlString = urlString.replace("%p", QString().number(page));
	}
	urlString = fpar.forumUrlWithoutEnd() + urlString;
	return urlString;
}

QString ForumSession::getMessageListUrl(const ForumThread *thread, int page) {
	QString urlString = fpar.view_thread_path;
	urlString = urlString.replace("%g", thread->group()->id());
	urlString = urlString.replace("%t", thread->id());
	if (fpar.supportsMessagePages()) {
		if (page < 0)
			page = fpar.view_thread_page_start;
		urlString = urlString.replace("%p", QString().number(page));
	}
	urlString = fpar.forumUrlWithoutEnd() + urlString;
	return urlString;
}

void ForumSession::authenticationRequired(QNetworkReply * reply,
		QAuthenticator * authenticator) {
	qDebug() << Q_FUNC_INFO;

	if (fsub->username().length() <= 0 || fsub->password().length() <= 0) {
		qDebug() << "FAIL: no credentials given for subscription "
				<< fsub->toString();
		cancelOperation();
		emit networkFailure(
				"Server requested for username and password for forum "
						+ fsub->name() + " but you haven't provided them.");
	} else {
		qDebug() << "Gave credentials to server";
		authenticator->setUser(fsub->username());
		authenticator->setPassword(fsub->password());
	}
}

void ForumSession::clearAuthentications() {
	qDebug() << Q_FUNC_INFO;
	cancelOperation();

	if (cookieJar)
		cookieJar->deleteLater();
	if (nam)
		nam->deleteLater();
	loggedIn = false;

	nam = new QNetworkAccessManager(this);
	cookieJar = new QNetworkCookieJar();
	nam->setCookieJar(cookieJar);
	connect(nam, SIGNAL(authenticationRequired(QNetworkReply *,
					QAuthenticator *)), this,
			SLOT(authenticationRequired(QNetworkReply *,
							QAuthenticator *)));
}

bool ForumSession::prepareForUse() {
	qDebug() << Q_FUNC_INFO;
	if (!cookieFetched) {
		fetchCookie();
		return true;
	}
	if (!loggedIn && fpar.supportsLogin() && fsub->username().length() > 0 && fsub->password().length() > 0) {
		loginToForum();
		return true;
	}
	return false;
}

void ForumSession::nextOperation() {
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
