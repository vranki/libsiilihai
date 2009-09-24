#ifndef FORUMDATABASE_H_
#define FORUMDATABASE_H_
#include <QObject>
#include <QtSql>
#include <QList>
#include "forumparser.h"

class ForumDatabase : public QObject {
	Q_OBJECT
public:
	ForumDatabase(QObject *parent);
	virtual ~ForumDatabase();
	bool openDatabase( );
	bool addForum(int parser, QString name, QString username, QString password,
			int latest_threads, int latest_messages);

};

#endif /* FORUMDATABASE_H_ */
