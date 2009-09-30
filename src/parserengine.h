/*
 * parserengine.h
 *
 *  Created on: Sep 18, 2009
 *      Author: vranki
 */

#ifndef PARSERENGINE_H_
#define PARSERENGINE_H_
#include <QObject>
#include "forumparser.h"
#include "forumsubscription.h"
#include "forumsession.h"
#include "forumgroup.h"
#include "forumdatabase.h"

class ParserEngine : public QObject {
	Q_OBJECT

public:
	ParserEngine(ForumDatabase *fd, QObject *parent=0);
	virtual ~ParserEngine();
	void setParser(ForumParser &fp);
	void setSubscription(ForumSubscription &fs);
	void updateGroupList();
public slots:
	void listGroupsFinished(QList<ForumGroup> groups);
signals:
	void groupListChanged(int forum);
private:
	ForumParser parser;
	ForumSubscription subscription;
	ForumSession session;
	bool sessionInitialized;
	ForumDatabase *fdb;
};

#endif /* PARSERENGINE_H_ */
