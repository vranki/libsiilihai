#ifndef LIBSIILIHAITESTS_H_
#define LIBSIILIHAITESTS_H_

#include <QCoreApplication>
#include <QDebug>
#include <QTest>
#include "patternmatcher.h"
#include "parserengine.h"
#include "forumparser.h"
#include "siilihaiprotocol.h"
#include "parserdatabase.h"
#include "forumdatabase.h"

class LibSiilihaiTests : public QObject {
	Q_OBJECT
public:
	LibSiilihaiTests(QObject *parent=0);
	virtual ~LibSiilihaiTests();
signals:
	void testsFinished();
public slots:
	void runTests();
	void loginFinished(bool success);
	void listParsersFinished(QList <ForumParser> parsers);
	void getParserFinished(ForumParser parser);
private:
	void runParserEngineTests();
	void runProtocolTests();
	SiilihaiProtocol protocol;
	ParserDatabase pdb;
	ForumDatabase fdb;
	QSqlDatabase db;
};

#endif /* LIBSIILIHAITESTS_H_ */
