#include "forumgroup.h"

ForumGroup::ForumGroup(ForumSubscription *sub) : QObject(sub) {
    _subscription = sub;
    _id = "";
    _subscribed = false;
    _changeset = -1;
    _name = "";
    _lastchange = "";
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
    QString parser = "Unknown";
    if(subscription())
        parser = QString().number(subscription()->parser());
    return parser + "/" + _id + ": " + _name;
}

bool ForumGroup::isSane() const {
    return (_id.length() > 0 && _name.length() > 0 && _subscription);
}

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

void ForumGroup::setName(QString name) { _name = name; }
void ForumGroup::setId(QString id) { _id = id; }
void ForumGroup::setLastchange(QString lc) { _lastchange = lc; }
void ForumGroup::setSubscribed(bool s) { _subscribed = s; }
void ForumGroup::setChangeset(int cs) { _changeset = cs; }
