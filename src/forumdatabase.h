#ifndef FORUMDATABASE_H_
#define FORUMDATABASE_H_
#include <QObject>
#include <QtSql>
#include <QList>
#include "forumgroup.h"
#include "forumsubscription.h"

class ForumDatabase : public QObject {
	Q_OBJECT
public:
	ForumDatabase(QObject *parent);
	virtual ~ForumDatabase();
	bool openDatabase( );
	bool addForum(const ForumSubscription &fs);
	QList <ForumSubscription> listSubscriptions();
	QList <ForumGroup> listGroups(const int parser);
	bool addGroup(const ForumGroup &grp);
	bool deleteGroup(const ForumGroup &grp);
};

#endif /* FORUMDATABASE_H_ */
