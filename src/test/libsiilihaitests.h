/* This file is part of libSiilihai.

    libSiilihai is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libSiilihai is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with libSiilihai.  If not, see <http://www.gnu.org/licenses/>. */
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
#include "forumparser.h"
#include "forumsubscription.h"
#include "forumsession.h"
#include "parserengine.h"
#include "httppost.h"
#include "syncmaster.h"

class LibSiilihaiTests : public QObject {
	Q_OBJECT
public:
	LibSiilihaiTests(QObject *parent=0);
	virtual ~LibSiilihaiTests();
signals:
	void testsFinished();
public slots:
	void runTests();
	void loginFinished(bool success, QString motd);
	void listParsersFinished(QList <ForumParser> parsers);
	void getParserFinished(ForumParser parser);
	void subscribeGroupsFinished(bool success);
	void sendThreadDataFinished(bool success);
	void groupFound(ForumGroup *grp);
private:
	void runParserEngineTests();
	void runProtocolTests();
	void runForumSession();
	void runForumDatabaseTests();

	SiilihaiProtocol protocol;
	ForumDatabase fdb;
	ParserDatabase pdb;
	QSqlDatabase db;
	ForumParser fp;
	ForumSubscription fsub;
	ForumSession fses;
	ParserEngine engine;
	SyncMaster sm;
};

#endif /* LIBSIILIHAITESTS_H_ */
