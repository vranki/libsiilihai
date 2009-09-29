#ifndef FORUMGROUP_H_
#define FORUMGROUP_H_
#include <QString>

class ForumGroup {
public:
	ForumGroup();
	virtual ~ForumGroup();
	int parser;
	QString name;
	QString id;
	QString lastchange;
};

#endif
