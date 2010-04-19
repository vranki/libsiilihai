#include "forumthread.h"

ForumThread::ForumThread() : QObject() {
    _group = 0;
    _id = _name = _lastchange = QString::null;
    _changeset = -1;
    _ordernum = -1;
    _hasMoreMessages = _getAllMessages = false;
}

ForumThread::~ForumThread() {
}

ForumThread::ForumThread(ForumGroup *grp) : QObject(grp) {
    ForumThread();
    _group = grp;
}

ForumThread::ForumThread(const ForumThread& o) : QObject() {
    *this = o;
}

ForumThread& ForumThread::operator=(const ForumThread& o) {
    _group = o._group;
    _id = o._id;
    _name = o._name;
    _lastchange = o._lastchange;
    _changeset = o._changeset;
    _ordernum = o._ordernum;

    return *this;
}


QString ForumThread::toString() const {
    QString tparser = "Unknown";
    QString tgroup = "Unknown";
    if(group()) {
        tgroup = group()->id();
        if(group()->subscription())
            tparser = QString().number(group()->subscription()->parser());
    }
    return tparser + "/" + tgroup +
            "/" + id() + ": " + name();
}

bool ForumThread::isSane() const {
    return (_group && _id.length() > 0);
}

QString ForumThread::id() const {
    return _id;
}
int ForumThread::ordernum() const {
    return _ordernum;
}
QString ForumThread::name() const {
    return _name;
}
QString ForumThread::lastchange() const {
    return _lastchange;
}
int ForumThread::changeset() const {
    return _changeset;
}
ForumGroup *ForumThread::group() const {
    return _group;
}

bool ForumThread::hasMoreMessages() const {
    return _hasMoreMessages;
}

bool ForumThread::getAllMessages() const {
    return _getAllMessages;
}

void ForumThread::setId(QString nid) {
    _id = nid;
}
void ForumThread::setOrdernum(int on) {
    _ordernum = on;
}
void ForumThread::setName(QString nname) {
    _name = nname;
}
void ForumThread::setLastchange(QString lc) {
    _lastchange = lc ;
}
void ForumThread::setChangeset(int cs) {
    _changeset = cs;
}

void ForumThread::setHasMoreMessages(bool hmm) {
    _hasMoreMessages = hmm;
}

void ForumThread::setGetAllMessages(bool gam) {
    _getAllMessages = gam;
}
