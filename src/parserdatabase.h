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
#ifndef PARSERDATABASE_H_
#define PARSERDATABASE_H_
#include <QObject>
#include <QtSql>
#include <QList>
#include "forumparser.h"

class ParserDatabase : public QObject {
	Q_OBJECT

public:
	ParserDatabase(QObject *parent);
	virtual ~ParserDatabase();
	bool openDatabase( );
	bool storeParser(const ForumParser &p );
	void deleteParser(const int id);
	ForumParser getParser(const int id);
	QList <ForumParser> listParsers();
private:
};

#endif /* PARSERDATABASE_H_ */
