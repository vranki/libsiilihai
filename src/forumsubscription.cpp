#include "forumsubscription.h"

ForumSubscription::ForumSubscription() : QObject() {
	_parser = -1;
	_name = QString::null;
	_latestThreads = 0;
	_latestMessages = 0;
}

ForumSubscription::ForumSubscription(QObject *parent) : QObject(parent) {
	ForumSubscription();
}

ForumSubscription& ForumSubscription::operator=(const ForumSubscription& other) {
	_parser = other._parser;
	_name = other._name;
	_username = other._username;
	_password = other._password;
	_latestThreads = other._latestThreads;
	_latestMessages = other._latestMessages;

	return *this;
}

ForumSubscription::ForumSubscription(const ForumSubscription& other) : QObject() {
	*this = other;
}

ForumSubscription::~ForumSubscription() {
}

bool ForumSubscription::isSane() const {
	return (_parser > 0 && _name.length() > 0 && _latestMessages > 0 && _latestThreads > 0);
}

QString ForumSubscription::toString() const {
	return QString("Subscription to ") + QString().number(_parser) + " (" + _name + ")";
}

int ForumSubscription::parser() const {
	return _parser;
}
QString ForumSubscription::name() const {
	return _name;
}
QString ForumSubscription::username() const {
	return _username;
}
QString ForumSubscription::password() const {
	return _password;
}
unsigned int ForumSubscription::latest_threads() const {
	return _latestThreads;
}
unsigned int ForumSubscription::latest_messages() const {
	return _latestMessages;
}
void ForumSubscription::setParser(int parser) {
	_parser = parser;
}
void ForumSubscription::setName(QString name) {
	_name = name;
}
void ForumSubscription::setUsername(QString username) {
	_username = username;
}
void ForumSubscription::setPassword(QString password) {
	_password = password;
}
void ForumSubscription::setLatestThreads(unsigned int lt) {
	_latestThreads = lt;
}
void ForumSubscription::setLatestMessages(unsigned int lm) {
	_latestMessages = lm;
}

/*
QList <ForumGroup*> ForumSubscription::children() const {
	return _groups;
}

void ForumSubscription::addGroup(ForumGroup *grp) {
	qDebug() << Q_FUNC_INFO;
	// Q_ASSERT(!_groups.contains(grp->id()));
	_groups.append(grp);
	// grp->setSubscription(this);
	emit groupAdded(grp);
}

void ForumSubscription::deleteGroup(ForumGroup *grp) {
	qDebug() << Q_FUNC_INFO;
	Q_ASSERT(_groups.contains(grp->id()));

	_groups.erase(_groups.find(grp->id()));
	emit groupDeleted(grp);
}
*/
