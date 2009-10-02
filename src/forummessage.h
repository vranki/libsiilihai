#ifndef FORUMMESSAGE_H_
#define FORUMMESSAGE_H_
#include <QString>

class ForumMessage {
public:
	ForumMessage();
	virtual ~ForumMessage();
	bool isSane() const;
	QString toString() const;
	int forumid;
	QString groupid;
	QString threadid;
	QString id;
	int ordernum;
	QString url;
	QString subject;
	QString author;
	QString lastchange;
	QString body;
	bool read;
};

#endif /* FORUMMESSAGE_H_ */
