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

ForumThread::ForumThread(ForumGroup *grp, bool temp) : ForumDataItem(grp) {
    _group = grp;
    _changeset = -1;
    _ordernum = -1;
    _hasMoreMessages = false;
    _getMessagesCount = -1;
    _hasChanged = false;
    _temp = temp;
    _lastPage = 0;
}

void ForumThread::copyFrom(ForumThread * o) {
    setId(o->id());
    setName(o->name());
    setLastchange(o->lastchange());
    setChangeset(o->changeset());
    setOrdernum(o->ordernum());
    setHasMoreMessages(o->hasMoreMessages());
    setGetMessagesCount(o->getMessagesCount());
    setLastPage(o->getLastPage());
}

bool ForumThread::operator<(const ForumThread &o) {
   return ordernum() < o.ordernum();
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
    return (_group && id().length() > 0 && _getMessagesCount >= 0);
}


int ForumThread::ordernum() const {
    return _ordernum;
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


int ForumThread::getMessagesCount() const {
    return _getMessagesCount;
}

void ForumThread::setOrdernum(int on) {
if(on == _ordernum) return;
    _ordernum = on;
    _propertiesChanged = true;
}
void ForumThread::setChangeset(int cs) {
if(cs==_changeset) return;
    _changeset = cs;
    _propertiesChanged = true;
}

void ForumThread::setHasMoreMessages(bool hmm) {
if(hmm==_hasMoreMessages) return;
    _hasMoreMessages = hmm;
    _propertiesChanged = true;
}

void ForumThread::setGetMessagesCount(int gmc) {
if(gmc==_getMessagesCount) return;
    _getMessagesCount = gmc;
    _propertiesChanged = true;
}


QMap<QString, ForumMessage*> & ForumThread::messages() {
    return _messages;
}
bool ForumThread::isTemp() {
   return _temp;
}


void ForumThread::emitChanged() {
    emit changed(this);
}

void ForumThread::emitUnreadCountChanged() {
    emit unreadCountChanged(this);
}

void ForumThread::setLastPage(int lp) {
if(_lastPage == lp) return;
    _lastPage = lp;
    _propertiesChanged = true;
}

int ForumThread::getLastPage() {
    return _lastPage;
}
