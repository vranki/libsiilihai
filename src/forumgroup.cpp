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

#include "forumgroup.h"


ForumGroup::ForumGroup(ForumSubscription *sub, bool temp) : QObject(sub) {
    _subscription = sub;
    _id = "";
    _subscribed = false;
    _changeset = -1;
    _name = "";
    _lastchange = "";
    _hasChanged = false;
    _unreadCount = 0;
    _temp = temp;
}

void ForumGroup::copyFrom(ForumGroup * o) {
    setId(o->id());
    setName(o->name());
    setLastchange(o->lastchange());
    setSubscribed(o->subscribed());
    setChangeset(o->changeset());
    setHasChanged(o->hasChanged());
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

bool ForumGroup::hasChanged() const {
    return _hasChanged;
}
int ForumGroup::unreadCount() const {
    return _unreadCount;
}

void ForumGroup::setName(QString name) {
  if(name == _name) return;
  _name = name;
  emit changed(this);
}

void ForumGroup::setId(QString id) {
if(_id == id) return;
_id = id;
emit changed(this);
}

void ForumGroup::setLastchange(QString lc) {
if(lc==_lastchange) return;
_lastchange = lc;
emit changed(this);
}

void ForumGroup::setSubscribed(bool s) {
if(s==_subscribed) return;
_subscribed = s;
emit changed(this);
}

void ForumGroup::setChangeset(int cs) {
if(cs==_changeset) return;
_changeset = cs;
emit changed(this);
}

void ForumGroup::setHasChanged(bool hc) {
if(hc==_hasChanged) return;
   _hasChanged = hc;
   emit changed(this);
}

void ForumGroup::incrementUnreadCount(int urc) {
    if(!urc) return;
    _unreadCount += urc;
    // Q_ASSERT(_unreadCount >= 0);
    subscription()->incrementUnreadCount(urc);
    emit unreadCountChanged(this);
}

QMap<QString, ForumThread*> & ForumGroup::threads() {
   return _threads;
}
bool ForumGroup::isTemp() {
return _temp;
}
