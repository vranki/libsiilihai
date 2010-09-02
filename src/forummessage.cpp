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

ForumMessage::~ForumMessage() {
}

ForumMessage::ForumMessage(ForumThread *thr, bool temp) : ForumDataItem(thr) {
    _thread = thr;
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
    setRead(o->isRead());
}

bool ForumMessage::operator<(const ForumMessage &o) {
   return ordernum() < o.ordernum();
}

bool ForumMessage::isSane() const {
    return (_thread && id().length()>0);
}

QString ForumMessage::toString() const {
return "MSG";/*
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
            _thread->group()->id() + "/" + _thread->id() + "/" + id() + ": " + subject() + "/ Read:" + QString().number(_read);
*/
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
  emit changed(this);
}

void ForumMessage::setUrl(QString nurl) {
if(nurl == _url) return;
_url = nurl;
  emit changed(this);
}
void ForumMessage::setAuthor(QString na) {
if(na == _author) return;
_author = na;
  emit changed(this);
}
void ForumMessage::setBody(QString nb) {
  if(nb == _body) return;
  _body = nb ;
  emit changed(this);
}
void ForumMessage::setRead(bool nr) {
  if(nr==_read) return;
  _read = nr;
  int dsubs = 1;
  if(_read)
      dsubs = -1;
  thread()->incrementUnreadCount(dsubs);
  thread()->group()->incrementUnreadCount(dsubs);
  thread()->group()->subscription()->incrementUnreadCount(dsubs);
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
