#ifndef FORUMTHREAD_H_
#define FORUMTHREAD_H_
#include <QString>
#include <QObject>
#include "forumgroup.h"

class ForumThread : public QObject {
	Q_OBJECT
public:
	ForumThread();
	ForumThread(ForumGroup *grp);
	virtual ~ForumThread();
	ForumThread& operator=(const ForumThread&);
	ForumThread(const ForumThread&);

	QString id() const;
	int ordernum() const;
	QString name() const;
	QString lastchange() const;
	int changeset() const;
	ForumGroup *group() const;

	void setId(QString nid);
	void setOrdernum(int on);
	void setName(QString nname);
	void setLastchange(QString lc);
	void setChangeset(int cs);

	QString toString() const;
	bool isSane() const;

private:
	int _forumid;
	QString _groupid;
	QString _id;
	int _ordernum;
	QString _name;
	QString _lastchange;
	int _changeset;
	ForumGroup *_group;
};

#endif /* FORUMTHREAD_H_ */
