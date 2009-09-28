#ifndef FORUMDATABASE_H_
#define FORUMDATABASE_H_
#include <QObject>
#include <QtSql>
#include <QList>
// #include "forumparser.h"
#include "forumsubscription.h"

class ForumDatabase : public QObject {
	Q_OBJECT
public:
	ForumDatabase(QObject *parent);
	virtual ~ForumDatabase();
	bool openDatabase( );
	bool addForum(const ForumSubscription &fs);
	QList <ForumSubscription> listSubscriptions();
};

#endif /* FORUMDATABASE_H_ */
