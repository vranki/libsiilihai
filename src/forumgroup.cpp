#include "forumgroup.h"

ForumGroup::ForumGroup() : QObject() {
	_id = QString::null;
	_subscribed = false;
	_changeset = -1;
	_subscription = 0;
}
/*
ForumGroup::ForumGroup(QObject *parent) : QObject(parent){
	ForumGroup();
}
*/
ForumGroup::ForumGroup(ForumSubscription *sub) : QObject(sub) {
	ForumGroup();
	_subscription = sub;
}

ForumGroup::ForumGroup(const ForumGroup& o) : QObject() {
	*this = o;
}

ForumGroup& ForumGroup::operator=(const ForumGroup& o) {
	_subscription = o._subscription;
	_id = o._id;
	_name = o._name;
	_lastchange = o._lastchange;
	_subscribed = o._subscribed;
	_changeset = o._changeset;

	return *this;
}

ForumGroup::~ForumGroup() {
}

QString ForumGroup::toString() const {
	return QString().number(subscription()->parser()) + "/" + _id + ": " + _name;
}

bool ForumGroup::isSane() const {
	return (_id.length() > 0 && _name.length() > 0 && _subscription);
}
/*
ForumSubscription* ForumGroup::subscription() const {
	return _subscription;
}
*/
QString ForumGroup::name() const {
	return _name;
}

QString ForumGroup::id() const {
	return _id;
}
QString ForumGroup::lastchange() const {
	return _lastchange;
}
bool ForumGroup::subscribed() const {
	return _subscribed;
}
int ForumGroup::changeset() const {
	return _changeset;
}

ForumSubscription* ForumGroup::subscription() const {
	return _subscription;
}

//void ForumGroup::setSubscription(ForumSubscription *sub) { _subscription = sub; }
void ForumGroup::setName(QString name) { _name = name; }
void ForumGroup::setId(QString id) { _id = id; }
void ForumGroup::setLastchange(QString lc) { _lastchange = lc; }
void ForumGroup::setSubscribed(bool s) { _subscribed = s; }
void ForumGroup::setChangeset(int cs) { _changeset = cs; }
