#ifndef FORUMSUBSCRIPTION_H_
#define FORUMSUBSCRIPTION_H_
#include <QString>
#include <QObject>
#include <QMap>
#include <QSet>
#include <QDebug>
// #include "forumgroup.h"

// class ForumGroup;

class ForumSubscription : public QObject {
	Q_OBJECT

public:
	ForumSubscription();
	ForumSubscription(QObject *parent);
	ForumSubscription(const ForumSubscription&);
	ForumSubscription&  operator=(const ForumSubscription&);
	virtual ~ForumSubscription();
	bool isSane() const;
	QString toString() const;
	void setParser(int parser);
        void setAlias(QString alias);
	void setUsername(QString username);
	void setPassword(QString password);
	void setLatestThreads(unsigned int lt);
	void setLatestMessages(unsigned int lm);
        void setAuthenticated(bool na);
	int parser() const;
        QString alias() const;
	QString username() const;
	QString password() const;
	unsigned int latest_threads() const;
	unsigned int latest_messages() const;
        bool authenticated() const; // True if username & password should be set
private:
	int _parser;
        QString _alias;
	QString _username;
	QString _password;
	unsigned int _latestThreads;
	unsigned int _latestMessages;
        bool _authenticated;
};

#endif /* FORUMSUBSCRIPTION_H_ */
