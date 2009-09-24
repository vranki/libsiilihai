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
