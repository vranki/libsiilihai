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

ForumMessage::ForumMessage(ForumThread *thr) : QObject(thr) {
    _thread = thr;
    _id = _subject = _author = _body = _lastchange = "";
    _ordernum = -1;
    _read = false;
}
void ForumMessage::copyFrom(ForumMessage * o) {
Q_ASSERT(o->id() == id());
    setId(o->id());
    setOrdernum(o->ordernum());
    setUrl(o->url());
    setSubject(o->subject());
    setAuthor(o->author());
    setLastchange(o->lastchange());
    setBody(o->body());
    setRead(o->read());
}
/*
ForumMessage::ForumMessage(const ForumMessage& o) : QObject() {
    *this = o;
}
*/
bool ForumMessage::isSane() const {
    return (_thread && _id.length()>0 && _ordernum < 1000);
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
            _thread->group()->id() + "/" + _thread->id() + "/" + id() + ": " + subject() + "/ Read:" + QString().number(_read);
}

ForumThread* ForumMessage::thread() const { return _thread; }
QString ForumMessage::id() const { return _id; }
int ForumMessage::ordernum() const { return _ordernum; }
QString ForumMessage::url() const { return _url; }
QString ForumMessage::subject() const { return _subject; }
QString ForumMessage::author() const { return _author; }
QString ForumMessage::lastchange() const { return _lastchange; }
QString ForumMessage::body() const { return _body; }
bool ForumMessage::read() const { return _read; }

void ForumMessage::setId(QString nid) {
  if(nid == _id) return;
  _id = nid ;
  emit changed(this);
}

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
void ForumMessage::setSubject(QString ns) {
if(ns == _subject) return;
  _subject = ns;
  emit changed(this);
 }
void ForumMessage::setAuthor(QString na) {
if(na == _author) return;
_author = na;
  emit changed(this);
}
void ForumMessage::setLastchange(QString nlc) {
if(nlc==_lastchange) return;
_lastchange = nlc ;
  emit changed(this);
}
void ForumMessage::setBody(QString nb) {
  if(nb == _body) return;
  _body = nb ;
  emit changed(this);
}
void ForumMessage::setRead(bool nr) {
  if(nr==_read) return;
  _read = nr ;
  if(_read) {
     thread()->incrementUnreadCount(-1);
   } else {
     thread()->incrementUnreadCount(1);
  }
  emit markedRead(this, nr);
}

void ForumMessage::setThread(ForumThread *nt) {
  if(nt==_thread) return;
  _thread = nt;
  emit changed(this);
}
