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
#include <QList>
#include <QMap>
#include <QIODevice>
#include <QFile>
#include "forumparser.h"

/**
  * Stores ForumParsers in a local database.
  *
  * @see ForumParser
  */
class ParserDatabase : public QObject, public QMap<int, ForumParser*>  {
    Q_OBJECT

public:
    ParserDatabase(QObject *parent);
    virtual ~ParserDatabase();
    bool storeDatabase();
    bool openDatabase(QIODevice *source);
    bool openDatabase(QString filename);
    bool storeParser(const ForumParser &p );
    void deleteParser(const int id);
private:
    QString databaseFileName;
};

#endif /* PARSERDATABASE_H_ */
