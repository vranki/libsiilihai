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

ForumThread::ForumThread(ForumGroup *grp) : QObject(grp) {
    _group = grp;
    _id = _name = _lastchange = "";
    _changeset = -1;
    _ordernum = -1;
    _hasMoreMessages = false;
    _getMessagesCount = -1;
    _hasChanged = false;
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
    _hasMoreMessages = o._hasMoreMessages;
    _getMessagesCount = o._getMessagesCount;
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
    return (_group && _id.length() > 0 && _ordernum < 10000 && _getMessagesCount >= 0);
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

bool ForumThread::hasChanged() const {
    return _hasChanged;
}

int ForumThread::getMessagesCount() const {
    return _getMessagesCount;
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

void ForumThread::setGetMessagesCount(int gmc) {
    _getMessagesCount = gmc;
}
void ForumThread::setGroup(ForumGroup *ng) {
    _group = ng;
}

void ForumThread::setHasChanged(bool hc) {
    _hasChanged = hc;
}
