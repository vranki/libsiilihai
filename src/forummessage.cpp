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

ForumMessage& ForumMessage::operator=(const ForumMessage& o) {
    _id = o._id;
    _ordernum = o._ordernum;
    _url = o._url;
    _subject = o._subject;
    _author = o._author;
    _lastchange = o._lastchange;
    _body = o._body;
    _read = o._read;
    _thread = o._thread;

    return *this;
}

ForumMessage::ForumMessage(const ForumMessage& o) : QObject() {
    *this = o;
}

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

void ForumMessage::setId(QString nid) { _id = nid ; }
void ForumMessage::setOrdernum(int nod) { _ordernum = nod ; }
void ForumMessage::setUrl(QString nurl) { _url = nurl ; }
void ForumMessage::setSubject(QString ns) { _subject = ns ; }
void ForumMessage::setAuthor(QString na) { _author = na ; }
void ForumMessage::setLastchange(QString nlc) { _lastchange = nlc ; }
void ForumMessage::setBody(QString nb) { _body = nb ; }
void ForumMessage::setRead(bool nr) { _read = nr ; }
void ForumMessage::setThread(ForumThread *nt) { _thread = nt; };
