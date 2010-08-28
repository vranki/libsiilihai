/* This file is part of libSiilihai.

    libSiilihai is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libSiilihai is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with libSiilihai.  If not, see <http://www.gnu.org/licenses/>. */
#include "forumthread.h"

ForumThread::~ForumThread() {
}

ForumThread::ForumThread(ForumGroup *grp, bool temp) : QObject(grp) {
    _group = grp;
    _id = _name = _lastchange = "";
    _changeset = -1;
    _ordernum = -1;
    _hasMoreMessages = false;
    _getMessagesCount = -1;
    _hasChanged = false;
    _unreadCount = 0;
    _temp = temp;
}

void ForumThread::copyFrom(ForumThread * o) {
    setId(o->id());
    setName(o->name());
    setLastchange(o->lastchange());
    setChangeset(o->changeset());
    setOrdernum(o->ordernum());
    setHasMoreMessages(o->hasMoreMessages());
    setGetMessagesCount(o->getMessagesCount());
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
    return (_group && _id.length() > 0 && _getMessagesCount >= 0);
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

int ForumThread::unreadCount() const {
    return _unreadCount;
}

int ForumThread::getMessagesCount() const {
    return _getMessagesCount;
}

void ForumThread::setId(QString nid) {
    if(nid == _id) return;
    _id = nid;
    emit changed(this);
}
void ForumThread::setOrdernum(int on) {
if(on == _ordernum) return;
    _ordernum = on;
    emit changed(this);
}
void ForumThread::setName(QString nname) {
if(nname == _name) return;
    _name = nname;
    emit changed(this);
}
void ForumThread::setLastchange(QString lc) {
if(lc==_lastchange) return;
    _lastchange = lc ;
    emit changed(this);
}
void ForumThread::setChangeset(int cs) {
if(cs==_changeset) return;
    _changeset = cs;
    emit changed(this);
}

void ForumThread::setHasMoreMessages(bool hmm) {
if(hmm==_hasMoreMessages) return;
    _hasMoreMessages = hmm;
    emit changed(this);
}

void ForumThread::setGetMessagesCount(int gmc) {
if(gmc==_getMessagesCount) return;
    _getMessagesCount = gmc;
    emit changed(this);
}
void ForumThread::setGroup(ForumGroup *ng) {
    _group = ng;
}

void ForumThread::incrementUnreadCount(int urc) {
    if(!urc) return;
    _unreadCount += urc;
    //Q_ASSERT(_unreadCount >= 0);
    group()->incrementUnreadCount(urc);
    emit unreadCountChanged(this);
}

QList<ForumMessage*> & ForumThread::messages() {
    return _messages;
}
bool ForumThread::isTemp() {
return _temp;
}
