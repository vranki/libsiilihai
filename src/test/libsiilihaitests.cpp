#include "libsiilihaitests.h"

LibSiilihaiTests::LibSiilihaiTests(QObject *parent) : QObject(parent), 	pdb(this), fdb(this)
 {
}

LibSiilihaiTests::~LibSiilihaiTests() {
}

void  LibSiilihaiTests::runParserEngineTests() {
	ParserEngine pe(this);
	ForumParser fp;

}

void  LibSiilihaiTests::runProtocolTests() {
	protocol.setBaseURL("http://localhost:8000/");
	connect(&protocol, SIGNAL(loginFinished(bool)), this, SLOT(loginFinished(bool)));
	protocol.login("keijjo", "keijjo");
}

void LibSiilihaiTests::loginFinished(bool success) {
	qDebug() << "Login success:" << success;
	connect(&protocol, SIGNAL(listParsersFinished(QList <ForumParser>)), this, SLOT(listParsersFinished(QList <ForumParser>)));
	protocol.listParsers();
}

void LibSiilihaiTests::listParsersFinished(QList <ForumParser> parsers) {
	qDebug() << "RX parsers" << parsers.size();
	if(parsers.size()>0) {
		connect(&protocol, SIGNAL(getParserFinished(ForumParser)), this, SLOT(getParserFinished(ForumParser)));
		protocol.getParser(parsers[0].id);
	}
}

void LibSiilihaiTests::getParserFinished(ForumParser parser) {
	qDebug() << "RX parser "<< parser.toString() << " which seems sane:" << parser.isSane();
	qDebug() << "Storing parser success: " << pdb.storeParser(parser);
	qDebug() << "List parsers returns: " << pdb.listParsers().size();
	qDebug() << "Deleting parser. ";
	pdb.deleteParser(pdb.listParsers().at(0).id);
}

void LibSiilihaiTests::runTests() {
	qDebug() << "Helloz";
	db = QSqlDatabase::addDatabase("QSQLITE");
	db.setDatabaseName("../siilihai.db");
	if (!db.open())
		qDebug() << "Unable to open db!";
	QVERIFY(pdb.openDatabase());
	QVERIFY(fdb.openDatabase());
	runProtocolTests();
	//runParserEngineTests();

	emit(testsFinished());
	return;

	QString
			html =
					"<body><test>foo%%foo</test>\n\tkekekee<num>123&x=y</num><test>keke</test>kekekee<num>666</num></body>";
	QString pattern = "<test>%a<%i<num>%B<";

	PatternMatcher pm(this);
	qDebug() << "begin test section:";
	QString kek;
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
	QList <QHash<QString, QString> > matches = pm.findMatches(html, pattern);
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

	emit(testsFinished());
}
