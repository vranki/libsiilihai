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


ForumGroup::ForumGroup(ForumSubscription *sub, bool temp) : ForumDataItem(sub) {
    _subscription = sub;
    _subscribed = false;
    _changeset = -1;
    _hasChanged = false;
    _temp = temp;
}

void ForumGroup::copyFrom(ForumGroup * o) {
    setId(o->id());
    setName(o->name());
    setLastchange(o->lastchange());
    setSubscribed(o->isSubscribed());
    setChangeset(o->changeset());
    setHasChanged(o->hasChanged());
}

ForumGroup::~ForumGroup() {
}

QString ForumGroup::toString() const {
    QString parser = "Unknown";
    if(subscription())
        parser = QString().number(subscription()->parser());
    return parser + "/" + id() + ": " + name();
}

bool ForumGroup::isSane() const {
    return (id().length() > 0 && name().length() > 0 && _subscription);
}

bool ForumGroup::isSubscribed() const {
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
void ForumGroup::setSubscribed(bool s) {
    if(s==_subscribed) return;
    _subscribed = s;
    _propertiesChanged = true;
}

void ForumGroup::setChangeset(int cs) {
    if(cs==_changeset) return;
    _changeset = cs;
    _propertiesChanged = true;
}

void ForumGroup::setHasChanged(bool hc) {
    if(hc==_hasChanged) return;
    _hasChanged = hc;
    _propertiesChanged = true; // @todo should we? this bool goes nowhere
}

QMap<QString, ForumThread*> & ForumGroup::threads() {
    return _threads;
}

bool ForumGroup::isTemp() {
    return _temp;
}

void ForumGroup::emitChanged() {
    emit changed(this);
}

void ForumGroup::emitUnreadCountChanged() {
    emit unreadCountChanged(this);
}

void ForumGroup::markToBeUpdated() {
    setLastchange("GROUP_UPDATE_NEEDED");
}
