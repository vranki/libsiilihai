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
#include "forummessage.h"
#include "forumgroup.h"
#include "forumthread.h"
#include "forumsubscription.h"

ForumMessage::~ForumMessage() {
}

ForumMessage::ForumMessage(ForumThread *thread, bool temp) : ForumDataItem(thread) {
    _thread = thread;
    _author = _body = "";
    _ordernum = -1;
    _read = true;
    _temp = temp;
}

void ForumMessage::copyFrom(ForumMessage * o) {
    setId(o->id());
    setOrdernum(o->ordernum());
    setUrl(o->url());
    setName(o->name());
    setAuthor(o->author());
    setLastchange(o->lastchange());
    setBody(o->body());
    setRead(o->isRead(), false);
}

bool ForumMessage::operator<(const ForumMessage &o) {
    return ordernum() < o.ordernum();
}

bool ForumMessage::isSane() const {
    return (_thread && id().length()>0);
}

QString ForumMessage::toString() const {
    QString parser = "Unknown";
    QString group = "Unknown";
    QString thread = "Unknown";
    if(_thread) {
        thread = _thread->id();
        if(_thread->group()) {
            group = _thread->group()->id();
            if(_thread->group()->subscription())
                parser = QString().number(_thread->group()->subscription()->parser());
        }
    }

    return QString().number(_thread->group()->subscription()->parser()) + "/" +
            _thread->group()->id() + "/" + _thread->id() + "/" + id() + ": " + name() + "/ Read:" + QString().number(_read);
}

ForumThread* ForumMessage::thread() const { return _thread; }
int ForumMessage::ordernum() const { return _ordernum; }
QString ForumMessage::url() const { return _url; }
QString ForumMessage::author() const { return _author; }

QString ForumMessage::body() const { return _body; }
bool ForumMessage::isRead() const { return _read; }

void ForumMessage::setOrdernum(int nod) {
    if(nod == _ordernum) return;
    _ordernum = nod;
    _propertiesChanged = true;
}

void ForumMessage::setUrl(QString nurl) {
    if(nurl == _url) return;
    _url = nurl;
    _propertiesChanged = true;
}
void ForumMessage::setAuthor(QString na) {
    if(na == _author) return;
    _author = na;
    _propertiesChanged = true;
}
void ForumMessage::setBody(QString nb) {
    if(nb == _body) return;
    _body = nb ;
    _propertiesChanged = true;
}

void ForumMessage::setRead(bool nr, bool affectsParents) {
    if(nr==_read) return;
    _read = nr;
    if(!affectsParents) return;
    if(thread()->group()->isSubscribed()) {
        if(_read) {
            thread()->incrementUnreadCount(-1);
            thread()->group()->incrementUnreadCount(-1);
            thread()->group()->subscription()->incrementUnreadCount(-1);
        } else {
            thread()->incrementUnreadCount(1);
            thread()->group()->incrementUnreadCount(1);
            thread()->group()->subscription()->incrementUnreadCount(1);
        }
        thread()->group()->setHasChanged(true);
    }
    _propertiesChanged = true;
    emit markedRead(this, nr);
}

bool ForumMessage::isTemp() {
    return _temp;
}

void ForumMessage::emitChanged() {
    emit changed(this);
}

void ForumMessage::emitUnreadCountChanged() {
}


void ForumMessage::markToBeUpdated(bool toBe) {
    UpdateableItem::markToBeUpdated(toBe);
    if(toBe) {
        thread()->markToBeUpdated();
    }
}
