#ifndef FORUMGROUP_H_
#define FORUMGROUP_H_
#include <QString>

class ForumGroup {
public:
	ForumGroup();
	virtual ~ForumGroup();
	QString toString() const;
	bool isSane() const;
	int parser;
	QString name;
	QString id;
	QString lastchange;
	bool subscribed;
	int changeset;
};

#endif
