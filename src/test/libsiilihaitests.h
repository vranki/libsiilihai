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
#include "siilihai/parser/patternmatcher.h"
#include "siilihai/parser/parserengine.h"
#include "siilihai/parser/forumparser.h"
#include "siilihai/siilihaiprotocol.h"
#include "siilihai/parser/parserdatabase.h"
#include "siilihai/forumdatabase/forumdatabasexml.h"
#include "siilihai/parser/forumparser.h"
#include "siilihai/forumdata/forumsubscription.h"
#include "siilihai/parser/forumsession.h"
#include "siilihai/parser/parserengine.h"
#include "siilihai/httppost.h"
#include "siilihai/syncmaster.h"

class LibSiilihaiTests : public QObject {
	Q_OBJECT
public:
	LibSiilihaiTests(QObject *parent=0);
	virtual ~LibSiilihaiTests();
signals:
	void testsFinished();
public slots:
	void runTests();
	void groupFound(ForumGroup *grp);
private:
	void runForumDatabaseTests();

	SiilihaiProtocol protocol;
        ForumDatabaseXml fdb;
};

#endif /* LIBSIILIHAITESTS_H_ */
