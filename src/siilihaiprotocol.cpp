#include "siilihaiprotocol.h"

SiilihaiProtocol::SiilihaiProtocol(QObject *parent) :
	QObject(parent) {
	nam.setCookieJar(new QNetworkCookieJar(this));
}

SiilihaiProtocol::~SiilihaiProtocol() {
}

void SiilihaiProtocol::listParsers() {
	QNetworkRequest req(listParsersUrl);
	QHash<QString, QString> params;
	if (!clientKey.isNull()) {
		params.insert("client_key", clientKey);
	}
	listParsersData = HttpPost::setPostParameters(&req, params);
	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(replyListParsers(QNetworkReply*)));
	nam.post(req, listParsersData);
}

void SiilihaiProtocol::listSubscriptions() {
	QNetworkRequest req(listSubscriptionsUrl);
	QHash<QString, QString> params;
	if (!clientKey.isNull()) {
		params.insert("client_key", clientKey);
	}
	listSubscriptionsData = HttpPost::setPostParameters(&req, params);
	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(replyListSubscriptions(QNetworkReply*)));
	nam.post(req, listSubscriptionsData);
}

void SiilihaiProtocol::listRequests() {
	QNetworkRequest req(listRequestsUrl);
	QHash<QString, QString> params;
	if (!clientKey.isNull()) {
		params.insert("client_key", clientKey);
	}
	listRequestsData = HttpPost::setPostParameters(&req, params);
	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(replyListRequests(QNetworkReply*)));
	nam.post(req, listRequestsData);
}

void SiilihaiProtocol::login(QString user, QString pass) {
	QNetworkRequest req(loginUrl);
	QHash<QString, QString> params;
	params.insert("username", user);
	params.insert("password", pass);
	params.insert("action", "login");
	params.insert("clientversion", CLIENT_VERSION);
	loginData = HttpPost::setPostParameters(&req, params);
	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(replyLogin(QNetworkReply*)));
	nam.post(req, loginData);
}

void SiilihaiProtocol::registerUser(QString user, QString pass, QString email) {
	QNetworkRequest req(registerUrl);
	QHash<QString, QString> params;
	params.insert("username", user);
	params.insert("password", pass);
	params.insert("email", email);
	params.insert("captcha", "earth");
	params.insert("clientversion", CLIENT_VERSION);
	registerData = HttpPost::setPostParameters(&req, params);
	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(replyLogin(QNetworkReply*)));
	nam.post(req, registerData);
}

void SiilihaiProtocol::getParser(const int id) {
	QNetworkRequest req(getParserUrl);
	QHash<QString, QString> params;
	params.insert("id", QString().number(id));
	if (!clientKey.isNull()) {
		params.insert("client_key", clientKey);
	}

	getParserData = HttpPost::setPostParameters(&req, params);
	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(replyGetParser(QNetworkReply*)));
	nam.post(req, getParserData);
}

void SiilihaiProtocol::saveParser(const ForumParser &parser) {
	if (!parser.mayWork()) {
		qDebug() << "Tried to save not working parser!!";
		emit
		(saveParserFinished(-666, "That won't work!"));
		return;
	}
	QNetworkRequest req(saveParserUrl);
	QHash<QString, QString> params;
	params.insert("id", QString().number(parser.id));
	params.insert("parser_name", parser.parser_name);
	params.insert("forum_url", parser.forum_url);
	params.insert("parser_status", QString().number(parser.parser_status));
	params.insert("thread_list_path", parser.thread_list_path);
	params.insert("view_thread_path", parser.view_thread_path);
	params.insert("login_path", parser.login_path);
	params.insert("date_format", QString().number(parser.date_format));
	params.insert("group_list_pattern", parser.group_list_pattern);
	params.insert("thread_list_pattern", parser.thread_list_pattern);
	params.insert("message_list_pattern", parser.message_list_pattern);
	params.insert("verify_login_pattern", parser.verify_login_pattern);
	params.insert("login_parameters", parser.login_parameters);
	params.insert("login_type", QString().number(parser.login_type));
	params.insert("charset", parser.charset.toLower());
	params.insert("thread_list_page_start", QString().number(
			parser.thread_list_page_start));
	params.insert("thread_list_page_increment", QString().number(
			parser.thread_list_page_increment));
	params.insert("view_thread_page_start", QString().number(
			parser.view_thread_page_start));
	params.insert("view_thread_page_increment", QString().number(
			parser.view_thread_page_increment));
	params.insert("forum_software", parser.forum_software);
	params.insert("view_message_path", parser.view_message_path);
	params.insert("parser_type", QString().number(parser.parser_type));
	params.insert("posting_path", parser.posting_path);
	params.insert("posting_subject", parser.posting_subject);
	params.insert("posting_message", parser.posting_message);
	params.insert("posting_parameters", parser.posting_parameters);
	params.insert("posting_hints", parser.posting_hints);

	if (!clientKey.isNull()) {
		params.insert("client_key", clientKey);
	}

	saveParserData = HttpPost::setPostParameters(&req, params);
	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(replySaveParser(QNetworkReply*)));
	nam.post(req, saveParserData);
}

void SiilihaiProtocol::setBaseURL(QString &bu) {
	baseUrl = bu;
	listParsersUrl = QUrl(baseUrl + "api/forumlist.xml");
	loginUrl = QUrl(baseUrl + "api/login.xml");
	registerUrl = QUrl(baseUrl + "api/register.xml");
	getParserUrl = QUrl(baseUrl + "api/getparser.xml");
	subscribeForumUrl = QUrl(baseUrl + "api/subscribeforum.xml");
	saveParserUrl = QUrl(baseUrl + "api/saveparser.xml");
	listRequestsUrl = QUrl(baseUrl + "api/requestlist.xml");
	listSubscriptionsUrl = QUrl(baseUrl + "api/subscriptionlist.xml");
	sendParserReportUrl = QUrl(baseUrl + "api/sendparserreport.xml");
	subscribeGroupsUrl = QUrl(baseUrl + "api/subscribegroups.xml");
	sendThreadDataUrl = QUrl(baseUrl + "api/threaddata.xml");
	syncSummaryUrl = QUrl(baseUrl + "api/syncsummary.xml");
	getThreadDataUrl = syncSummaryUrl;//QUrl(baseUrl + "api/getthreaddata.xml");
	nam.setProxy(QNetworkProxy::applicationProxy());
}

void SiilihaiProtocol::subscribeForum(const ForumSubscription *fs,
		bool unsubscribe) {
	QNetworkRequest req(subscribeForumUrl);
        QHash<QString, QString> params;
        params.insert("parser_id", QString().number(fs->parser()));
	if (unsubscribe) {
		params.insert("unsubscribe", "yes");
	} else {
                params.insert("latest_threads", QString().number(fs->latest_threads()));
                params.insert("latest_messages", QString().number(fs->latest_messages()));
	}
	if (!clientKey.isNull()) {
		params.insert("client_key", clientKey);
	}
	subscribeForumData = HttpPost::setPostParameters(&req, params);
	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(replySubscribeForum(QNetworkReply*)));
	nam.post(req, subscribeForumData);
}

void SiilihaiProtocol::replySubscribeForum(QNetworkReply *reply) {
	bool success = reply->error() == QNetworkReply::NoError;
	nam.disconnect(SIGNAL(finished(QNetworkReply*)));
	emit
	(subscribeForumFinished(success));
	reply->deleteLater();
}

void SiilihaiProtocol::subscribeGroups(QList<ForumGroup*> &fgs) {
	qDebug() << Q_FUNC_INFO;
	if (fgs.isEmpty()) {
		emit subscribeGroupsFinished(false);
		return;
	}
	ForumGroup *first = fgs[0];
	QNetworkRequest req(subscribeGroupsUrl);
	QDomDocument doc("SiilihaiML");
	QDomElement root = doc.createElement("SubscribeGroups");
	doc.appendChild(root);

	QDomElement forumTag = doc.createElement("forum");
	root.appendChild(forumTag);

	QDomText t = doc.createTextNode(QString().number(first->subscription()->parser()));
	forumTag.appendChild(t);

	foreach(ForumGroup *g, fgs)
		{
			QDomElement subTag;
			if (g->subscribed()) {
				subTag = doc.createElement("subscribe");
				subTag.setAttribute("changeset", g->changeset());
			} else {
				subTag = doc.createElement("unsubscribe");
			}
			root.appendChild(subTag);
			QDomText t = doc.createTextNode(g->id());
			subTag.appendChild(t);
		}
	QString xml = doc.toString();
	subscribeGroupsData = doc.toByteArray();
	qDebug() << "TX xml: " << xml;

	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(replySubscribeGroups(QNetworkReply*)));
	nam.post(req, subscribeGroupsData);
}

void SiilihaiProtocol::replySubscribeGroups(QNetworkReply *reply) {
	bool success = reply->error() == QNetworkReply::NoError;
	nam.disconnect(SIGNAL(finished(QNetworkReply*)));
	emit
	(subscribeGroupsFinished(success));
	reply->deleteLater();
}

void SiilihaiProtocol::sendThreadData(QList<ForumMessage*> &fms) {
	qDebug() << Q_FUNC_INFO;
	if (fms.isEmpty()) {
		emit sendThreadDataFinished(false);
	}
	ForumMessage *message = fms.first();
	QNetworkRequest req(sendThreadDataUrl);
	QDomDocument doc("SiilihaiML");
	QDomElement root = doc.createElement("ThreadData");
	doc.appendChild(root);

	QDomElement forumTag = doc.createElement("forum");
	root.appendChild(forumTag);

	QDomText t = doc.createTextNode(QString().number(message->thread()->group()->subscription()->parser()));
	forumTag.appendChild(t);

	QDomElement groupTag = doc.createElement("group");
	root.appendChild(groupTag);

	t = doc.createTextNode(message->thread()->group()->id());
	groupTag.appendChild(t);

	// Sort 'em to threads:
	QMap<QString, QList<ForumMessage*> > threadedMessages;
	for (int i = 0; i < fms.size(); i++) {
		if (fms[i]->read())
			threadedMessages[fms[i]->thread()->id()].append(fms[i]);
	}

	// Send thread data
	QMapIterator<QString, QList<ForumMessage*> > i(threadedMessages);
	while (i.hasNext()) {
		i.next();
		QDomElement threadTag = doc.createElement("thread");
		threadTag.setAttribute("id", i.key());
		threadTag.setAttribute("changeset", 0);
		ForumMessage *messagePtr;
		foreach(messagePtr, i.value())
			{
				if (messagePtr->read()) {
					QDomElement messageTag = doc.createElement("message");
					messageTag.setAttribute("id", messagePtr->id());
					threadTag.appendChild(messageTag);
				}
			}
		root.appendChild(threadTag);
	}
	QString xml = doc.toString();
	sendThreadDataData = doc.toByteArray();
	qDebug() << "TX xml: " << xml;

	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(replySendThreadData(QNetworkReply*)));
	nam.post(req, sendThreadDataData);
}

void SiilihaiProtocol::replySendThreadData(QNetworkReply *reply) {
	bool success = reply->error() == QNetworkReply::NoError;
	nam.disconnect(SIGNAL(finished(QNetworkReply*)));
	emit
	(sendThreadDataFinished(success));
	reply->deleteLater();
}

void SiilihaiProtocol::replyListParsers(QNetworkReply *reply) {
	QString docs = QString().fromUtf8(reply->readAll());
	QList<ForumParser> parsers;
	if (reply->error() == QNetworkReply::NoError) {
		QDomDocument doc;
		doc.setContent(docs);
		qDebug() << docs;
		QDomNode n = doc.firstChild().firstChild();
		while (!n.isNull()) {
			ForumParser parser;
			parser.id = QString(n.firstChildElement("id").text()).toInt();
			parser.forum_url = n.firstChildElement("forum_url").text();
			parser.parser_name = n.firstChildElement("name").text();
			parser.parser_status
					= QString(n.firstChildElement("status").text()).toInt();
			parser.parser_type = QString(
					n.firstChildElement("parser_type").text()).toInt();
			parsers.append(parser);
			n = n.nextSibling();
		}
	} else {
		qDebug() << "replyListParsers network error: " << reply->errorString();
	}
	nam.disconnect(SIGNAL(finished(QNetworkReply*)));
	emit
	(listParsersFinished(parsers));
	reply->deleteLater();
}

void SiilihaiProtocol::replyListSubscriptions(QNetworkReply *reply) {
	qDebug() << Q_FUNC_INFO;
	QString docs = QString().fromUtf8(reply->readAll());
	QList<int> subscriptions;
	if (reply->error() == QNetworkReply::NoError) {
		QDomDocument doc;
		doc.setContent(docs);
		qDebug() << docs;
		QDomNode n = doc.firstChild().firstChild();
		while (!n.isNull()) {
			int id = QString(n.toElement().text()).toInt();
			subscriptions.append(id);
			n = n.nextSibling();
		}
	} else {
		qDebug() << "replyListSubscriptions network error: "
				<< reply->errorString();
	}
	nam.disconnect(SIGNAL(finished(QNetworkReply*)));
	emit
	(listSubscriptionsFinished(subscriptions));
	reply->deleteLater();
}

void SiilihaiProtocol::replyListRequests(QNetworkReply *reply) {
	QString docs = QString().fromUtf8(reply->readAll());
	qDebug() << Q_FUNC_INFO << " " << docs;
	QList<ForumRequest> requests;
	if (reply->error() == QNetworkReply::NoError) {
		QDomDocument doc;
		doc.setContent(docs);
		QDomNode n = doc.firstChild().firstChild();
		while (!n.isNull()) {
			ForumRequest request;
			request.forum_url = n.firstChildElement("forum_url").text();
			request.comment = n.firstChildElement("comment").text();
			request.date = n.firstChildElement("date").text();
			request.user = n.firstChildElement("user").text();
			requests.append(request);
			n = n.nextSibling();
		}
	} else {
		qDebug() << "replyListRequests network error: " << reply->errorString();
	}
	nam.disconnect(SIGNAL(finished(QNetworkReply*)));
	emit
	(listRequestsFinished(requests));
	reply->deleteLater();
}

void SiilihaiProtocol::replyLogin(QNetworkReply *reply) {
	QString docs = QString().fromUtf8(reply->readAll());
	QString ck = QString::null;
	QString motd = QString::null;
	if (reply->error() == QNetworkReply::NoError) {
		QDomDocument doc;
		doc.setContent(docs);
		QDomElement re = doc.firstChild().toElement();
		ck = re.firstChildElement("client_key").text();
		motd = re.firstChildElement("motd").text();
	} else {
		qDebug() << "replyLogin network error: " << reply->errorString();
	}
	if (ck.length() > 0)
		clientKey = ck;
	nam.disconnect(SIGNAL(finished(QNetworkReply*)));
	emit
	(loginFinished(!clientKey.isNull(), motd));
	reply->deleteLater();
}

void SiilihaiProtocol::replyGetParser(QNetworkReply *reply) {
	QString docs = QString().fromUtf8(reply->readAll());
	ForumParser parser;
	parser.id = -1;
	qDebug() << docs;
	if (reply->error() == QNetworkReply::NoError) {
		QDomDocument doc;
		doc.setContent(docs);
		QDomElement re = doc.firstChild().toElement();
		parser.id = re.firstChildElement("id").text().toInt();
		parser.parser_name = re.firstChildElement("parser_name").text();
		parser.forum_url = re.firstChildElement("forum_url").text();
		parser.parser_status = re.firstChildElement("status").text().toInt();
		parser.thread_list_path
				= re.firstChildElement("thread_list_path").text();
		parser.view_thread_path
				= re.firstChildElement("view_thread_path").text();
		parser.login_path = re.firstChildElement("login_path").text();
		parser.date_format = re.firstChildElement("date_format").text().toInt();
		parser.group_list_pattern
				= re.firstChildElement("group_list_pattern").text();
		parser.thread_list_pattern
				= re.firstChildElement("thread_list_pattern").text();
		parser.message_list_pattern = re.firstChildElement(
				"message_list_pattern").text();
		parser.verify_login_pattern = re.firstChildElement(
				"verify_login_pattern").text();
		parser.login_parameters
				= re.firstChildElement("login_parameters").text();
		parser.login_type = (ForumParser::ForumLoginType) re.firstChildElement(
				"login_type").text().toInt();
		parser.charset = re.firstChildElement("charset").text().toLower();
		parser.thread_list_page_start = re.firstChildElement(
				"thread_list_page_start").text().toInt();
		parser.thread_list_page_increment = re.firstChildElement(
				"thread_list_page_increment").text().toInt();
		parser.view_thread_page_start = re.firstChildElement(
				"view_thread_page_start").text().toInt();
		parser.view_thread_page_increment = re.firstChildElement(
				"view_thread_page_increment").text().toInt();

		parser.forum_software = re.firstChildElement("forum_software").text();
		parser.view_message_path
				= re.firstChildElement("view_message_path").text();
		parser.parser_type = re.firstChildElement("parser_type").text().toInt();
		parser.posting_path = re.firstChildElement("posting_path").text();
		parser.posting_subject = re.firstChildElement("posting_subject").text();
		parser.posting_message = re.firstChildElement("posting_message").text();
		parser.posting_parameters
				= re.firstChildElement("posting_parameters").text();
		parser.posting_hints = re.firstChildElement("posting_hints").text();
	} else {
		qDebug() << "replyGetParser network error: " << reply->errorString();
	}
	nam.disconnect(SIGNAL(finished(QNetworkReply*)));
	emit
	(getParserFinished(parser));
	reply->deleteLater();
}

void SiilihaiProtocol::replySaveParser(QNetworkReply *reply) {
	QString docs = QString().fromUtf8(reply->readAll());
	int id = -1;
	QString msg = QString::null;
	qDebug() << docs;
	if (reply->error() == QNetworkReply::NoError) {
		QDomDocument doc;
		doc.setContent(docs);
		QDomElement re = doc.firstChild().toElement();
		id = re.firstChildElement("id").text().toInt();
		msg = re.firstChildElement("save_message").text();
	} else {
		qDebug() << "replySaveParser network error: " << reply->errorString();
		msg = reply->errorString();
	}
	nam.disconnect(SIGNAL(finished(QNetworkReply*)));
	emit
	(saveParserFinished(id, msg));
	reply->deleteLater();
}

void SiilihaiProtocol::replySendParserReport(QNetworkReply *reply) {
	QString docs = QString().fromUtf8(reply->readAll());
	bool success = false;
	qDebug() << docs;
	if (reply->error() == QNetworkReply::NoError) {
		QDomDocument doc;
		doc.setContent(docs);
		QString successStr = doc.firstChild().toElement().text();
		if (successStr == "true") {
			success = true;
		} else {
qDebug		() << Q_FUNC_INFO << " network error: " << reply->errorString();
	}
}
nam.disconnect(SIGNAL(finished(QNetworkReply*)));
emit
(sendParserReportFinished(success));
reply->deleteLater();
}

void SiilihaiProtocol::sendParserReport(ParserReport pr) {
	QNetworkRequest req(sendParserReportUrl);
	QHash<QString, QString> params;
	params.insert("parser_id", QString().number(pr.parserid));
	params.insert("type", QString().number(pr.type));
	params.insert("comment", pr.comment);

	if (!clientKey.isNull()) {
		params.insert("client_key", clientKey);
	}
	sendParserReportData = HttpPost::setPostParameters(&req, params);
	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(replySendParserReport(QNetworkReply*)));
	nam.post(req, sendParserReportData);
}

QString SiilihaiProtocol::baseURL() {
	return baseUrl;
}

void SiilihaiProtocol::getSyncSummary() {
	qDebug() << Q_FUNC_INFO;
	QNetworkRequest req(syncSummaryUrl);
	QHash<QString, QString> params;

	if (!clientKey.isNull()) {
		params.insert("client_key", clientKey);
	}
	syncSummaryData = HttpPost::setPostParameters(&req, params);
	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(replyGetSyncSummary(QNetworkReply*)));
	nam.post(req, syncSummaryData);
}

void SiilihaiProtocol::replyGetSyncSummary(QNetworkReply *reply) {
	QString docs = QString().fromUtf8(reply->readAll());
	qDebug() << Q_FUNC_INFO << docs;
	QList<ForumGroup> grps;
	/* @todo fix later
	if (reply->error() == QNetworkReply::NoError) {
		QDomDocument doc;
		doc.setContent(docs);
		QDomElement re = doc.firstChild().toElement();
		for (int i = 0; i < re.childNodes().size(); i++) {
			QDomElement forumElement = re.childNodes().at(i).toElement();
			int forumid = forumElement.attribute("id").toInt();
			for (int j = 0; j < forumElement.childNodes().size(); j++) {
				QDomElement groupElement =
						forumElement.childNodes().at(j).toElement();
				QString groupid = groupElement.attribute("id");
				int changeset = groupElement.text().toInt();
				ForumGroup g;
				g.parser = forumid;
				g.id = groupid;
				g.changeset = changeset;
				grps.append(g);
			}
		}

	} else {
		qDebug() << "replyGetSyncSummary network error: "
				<< reply->errorString();
	}
	*/
	nam.disconnect(SIGNAL(finished(QNetworkReply*)));
	reply->deleteLater();
	emit serverGroupStatus(grps);
}

void SiilihaiProtocol::getThreadData(ForumGroup *grp) {
	QNetworkRequest req(getThreadDataUrl);
	QHash<QString, QString> params;
	params.insert("forum_id", QString().number(grp->subscription()->parser()));
	params.insert("group_id", grp->id());

	if (!clientKey.isNull()) {
		params.insert("client_key", clientKey);
	}
	threadSummaryGroup = grp;
	getThreadDataData = HttpPost::setPostParameters(&req, params);
	connect(&nam, SIGNAL(finished(QNetworkReply*)), this,
			SLOT(replyGetThreadData(QNetworkReply*)));
	nam.post(req, getThreadDataData);
}

void SiilihaiProtocol::replyGetThreadData(QNetworkReply *reply) {
	qDebug() << Q_FUNC_INFO;
	QString docs = QString().fromUtf8(reply->readAll());
	qDebug() << docs;
	QMap<ForumThread, QList<ForumMessage> > threadData;

	if (reply->error() == QNetworkReply::NoError) {
		QDomDocument doc;
		doc.setContent(docs);
		QDomElement re = doc.firstChild().toElement();
		for (int j = 0; j < re.childNodes().size(); j++) {
			QDomElement forumElement = re.childNodes().at(j).toElement();
			int forumid = forumElement.attribute("id").toInt();
			for (int k = 0; k < forumElement.childNodes().size(); k++) {
				QDomElement groupElement =
						forumElement.childNodes().at(k).toElement();
				QString groupid = groupElement.attribute("id");
				int groupChangeset =
						groupElement.attribute("changeset").toInt();

				for (int l = 0; l < groupElement.childNodes().size(); l++) {
					QDomElement threadElement =
							groupElement.childNodes().at(l).toElement();
					QString threadid = threadElement.attribute("id");
					int threadChangeset =
							threadElement.attribute("changeset").toInt();
/* @todo fix
					ForumThread t;
					t.forumid = forumid;
					t.groupid = groupid;
					t.id = threadid;
					t.changeset = threadChangeset;
					emit serverThreadData(t);
					for (int m = 0; m < threadElement.childNodes().size(); m++) {
						QDomElement messageElement =
								threadElement.childNodes().at(m).toElement();
						QString messageid = messageElement.attribute("id");
						ForumMessage msg(thr);
						msg.setId(messageid);
						msg.setRead(true);
						emit serverMessageData(msg);
					}
*/
				}
			}
		}
	} else {
		qDebug() << "replyGetThreadSummary network error: "
				<< reply->errorString();
	}
	nam.disconnect(SIGNAL(finished(QNetworkReply*)));
	reply->deleteLater();
	emit getThreadDataFinished(reply->error() == QNetworkReply::NoError);
}
