#include "libsiilihaitests.h"

LibSiilihaiTests::LibSiilihaiTests(QObject *parent) :
	QObject(parent), fdb(parent), pdb(parent), fses(parent), engine(&fdb, parent), sm(parent, fdb, protocol) {
}

LibSiilihaiTests::~LibSiilihaiTests() {
}

void LibSiilihaiTests::runParserEngineTests() {
	qDebug("Parserenginetests..");
	engine.setParser(fp);
	engine.setSubscription(fsub);
	engine.updateForum(false);
}

void LibSiilihaiTests::runProtocolTests() {
	QString bu("http://localhost:8000/");
	protocol.setBaseURL(bu);
	connect(&protocol, SIGNAL(loginFinished(bool, QString)), this,
			SLOT(loginFinished(bool, QString)));
	protocol.login("keijjo", "keijjo");
	qDebug("Logging in..");
}

void LibSiilihaiTests::loginFinished(bool success, QString motd) {
	qDebug() << "Login success:" << success << " motd " << motd;
	connect(&protocol, SIGNAL(listParsersFinished(QList <ForumParser>)), this,
			SLOT(listParsersFinished(QList <ForumParser>)));
	// protocol.listParsers();
	//protocol.getSyncSummary();
	sm.startSync();
	//sm.endSync();
	/*
	connect(&protocol, SIGNAL(subscribeGroupsFinished(bool)), this,
			SLOT(subscribeGroupsFinished(bool)));
	ForumGroup fg;
	fg.parser = 2;
	fg.id = "kekk";
	QMap <bool, ForumGroup> fgs;
	fgs.insert(true, fg);
	fg.id = "jou";
	fgs.insert(false, fg);
	protocol.subscribeGroups(fgs);
	*/
}

void LibSiilihaiTests::subscribeGroupsFinished(bool success) {
	qDebug() << Q_FUNC_INFO << success;
	QList<ForumMessage> fms;
	ForumMessage fm;
	fm.forumid = 2;
	fm.groupid = "kekk";
	fm.threadid = "threadi";
	fm.id = "messaggi";
	fm.read = true;
	fms.append(fm);
	fm.id = "messaggi2";
	fm.read = false;
	fms.append(fm);
	fm.id = "messaggi3";
	fm.threadid = "toinenthreadi";
	fm.read = true;
	fms.append(fm);
	connect(&protocol, SIGNAL(sendThreadDataFinished(bool)), this,
			SLOT(sendThreadDataFinished(bool)));
	protocol.sendThreadData(fms);
}

void LibSiilihaiTests::sendThreadDataFinished(bool success) {
	qDebug() << Q_FUNC_INFO << success;
	emit(testsFinished());
}

void LibSiilihaiTests::listParsersFinished(QList<ForumParser> parsers) {
	qDebug() << "RX parsers" << parsers.size();
	if (parsers.size() > 0) {
		connect(&protocol, SIGNAL(getParserFinished(ForumParser)), this,
				SLOT(getParserFinished(ForumParser)));
		protocol.getParser(parsers[0].id);
	}
}

void LibSiilihaiTests::getParserFinished(ForumParser parser) {
	qDebug() << "RX parser " << parser.toString() << " which seems sane:"
			<< parser.isSane();
	qDebug() << "Storing parser success: " << pdb.storeParser(parser);
	qDebug() << "List parsers returns: " << pdb.listParsers().size();
	// qDebug() << "Deleting parser. ";
	// pdb.deleteParser(pdb.listParsers().at(0).id);
	runForumSession();
}

void LibSiilihaiTests::runForumSession() {
	qDebug() << "runForumSession() ";
	fp = pdb.listParsers().at(0);

	QVERIFY(fp.id > 0);
	if (fdb.listSubscriptions().size() == 0) {
		fsub.parser = fp.id;
		fsub.name = fp.parser_name;
		fsub.latest_threads = 10;
		fsub.latest_messages = 10;
		fsub.username = QString::null;
		fdb.addForum(fsub);
	}
	fsub = fdb.listSubscriptions()[0];
	runParserEngineTests();
	return;
	fses.initialize(fp, fsub);
	fses.listGroups();
}

void LibSiilihaiTests::runTests() {
	qDebug() << "Helloz";

	db = QSqlDatabase::addDatabase("QSQLITE");
	db.setDatabaseName(QDir::homePath() + "/.siilihai.db");
	if (!db.open()) {
		qDebug() << "Unable to open db!";
		return;
	}
	QVERIFY(pdb.openDatabase());
	QVERIFY(fdb.openDatabase());
	/*
	QHash<QString, QString> params;
	params["keke"] = "joo + jääöäöä 123 ;:;:&#/#%&/#¤&#&##&xxxx";
	params["pier"] = "true";
	HttpPost::setPostParameters(new QNetworkRequest(), params);
	*/
	runProtocolTests();

	QString
			html =
					"<body><test>foo%%foo</test>\n\tkekekee<num>123&x=y</num><test>keke</test>kekekee<num>666</num></body>";
	QString pattern = "<test>%a<%i<num>%B<";

	PatternMatcher pm(this);
	qDebug() << "begin test section:";
	QString kek;
	/*
	QVERIFY(pm.isTag(kek="%a"));
	QVERIFY(!pm.isTag(kek="%%"));
	QVERIFY(!pm.isTag(kek="%"));
	QVERIFY(!pm.isTag(kek=""));
	QVERIFY(!pm.isTag(kek="zttyztzyzy"));
	QVERIFY(!pm.isTag(kek="a%"));
	QVERIFY(pm.isNumberTag(kek="%A"));
	QVERIFY(!pm.isNumberTag(kek="%!"));
	QVERIFY(!pm.isNumberTag(kek="%9"));
	QVERIFY(pm.numberize(kek="aazhzh6j6xj6!¤!#¤#\"")=="6");
	QVERIFY(pm.numberize(kek="#&#/#%&/#%/#%/#/#%")=="");
	QVERIFY(pm.numberize(kek="123456789")=="123456789");
	QVERIFY(pm.numberize(kek="123&456789")=="123");
	QVERIFY(pm.tokenizePattern("%a%a").length()==0);
	QVERIFY(pm.tokenizePattern("%ax%a").length()==0);
	QVERIFY(pm.tokenizePattern("x%ax%ax").length()==5);
	*/
	qDebug() << "end test section";
	/*
	 QList<QString> patternTokens = pm.tokenizePattern(pattern);
	 qDebug() << "Pattern " << pattern << "\n tokenized:";
	 if (patternTokens.length() == 0) {
	 qDebug() << "invalid pattern!!";
	 } else {
	 for (int i = 0; i < patternTokens.length(); i++) {
	 qDebug() << "\t" << patternTokens[i];
	 }
	 }
	 */
	pm.setPattern(pattern);
	QList<QHash<QString, QString> > matches = pm.findMatches(html);
	for (int i = 0; i < matches.length(); i++) {
		qDebug() << "Match:";
		QHash<QString, QString> matchHash = matches[i];
		QHashIterator<QString, QString> i(matchHash);
		while (i.hasNext()) {
			i.next();
			qDebug() << "\t" << i.key() << ": " << i.value();
		}
	}

	/*
	 pattern = "<test>%a<%i<num>%%%B<";
	 pm.findMatches(html, pattern);
	 pattern = "<test>%a<%i\n<num>\n%%%B<%";
	 pm.findMatches(html, pattern);
	 */

}
