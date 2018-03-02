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

#include <QCoreApplication>

#include "forumgroup.h"
#include "forumthread.h"
#include "forumsubscription.h"
#include "forummessage.h"

ForumGroup::ForumGroup(QObject *parent, bool temp) :
    ForumDataItem(parent)
  ,  _subscription(nullptr)
  , _subscribed(false)
  , _changeset(-1)
  , _hasChanged(false)
  , _temp(temp)
{}

void ForumGroup::copyFrom(ForumGroup * o) {
    setId(o->id());
    setName(o->name());
    setLastChange(o->lastChange());
    setSubscribed(o->isSubscribed());
    setChangeset(o->changeset());
    setHasChanged(o->hasChanged());
    setHierarchy(o->hierarchy());
}

ForumGroup::~ForumGroup() { }

void ForumGroup::addThread(ForumThread* thr, bool affectsSync, bool incrementUnreads) {
    Q_ASSERT(!thr->group());
    Q_ASSERT(!contains(thr->id()));
    Q_ASSERT(thr->isTemp() == isTemp());

    if(incrementUnreads) incrementUnreadCount(thr->unreadCount());
    if(affectsSync) setHasChanged(true);
    thr->setGroup(this);
    append(thr);
    // Sort:
    std::sort(begin(), end(), [](ForumThread* a, ForumThread* b) {
        return a->ordernum() < b->ordernum();
    });
    emit threadAdded(thr);
}

void ForumGroup::removeThread(ForumThread* thr, bool affectsSync) {
    Q_ASSERT(thr->group() == this);
    // qDebug() << Q_FUNC_INFO << thr->toString();
    if(affectsSync && (thr->size() - thr->unreadCount()) > 0)  setHasChanged(true);
    int urc = thr->unreadCount();
    incrementUnreadCount(-urc);
    if(isSubscribed())
        subscription()->incrementUnreadCount(-urc);
    removeOne(thr);
    emit threadRemoved(thr);
    thr->deleteLater();
}

QString ForumGroup::toString() const {
    QString parser = "Unknown";
    if(subscription())
        parser = QString().number(subscription()->id());
    return parser + "/" + id() + ": " + name();
}

bool ForumGroup::contains(const QString &id) const
{
    return value(id);
}

ForumThread *ForumGroup::value(const QString &id) const
{
    for(ForumThread* thread : *this) {
        if(thread->id() == id) return thread;
    }
    return nullptr;
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

void ForumGroup::setSubscription(ForumSubscription *sub) {
    _subscription = sub;
}

void ForumGroup::setSubscribed(bool s) {
    if(s==_subscribed) return;

    // If not subscribed, remove all child threads
    if(!s) {
        while(!isEmpty())
            removeThread(last(), false);
    } else {
        subscription()->incrementUnreadCount(unreadCount());
    }
    _subscribed = s;
    _propertiesChanged = true;
    // @todo is this correct??
    /*
     no, i think
    if(_subscribed) {
        subscription()->incrementUnreadCount(unreadCount());
    } else {
        subscription()->incrementUnreadCount(-unreadCount());
        incrementUnreadCount(-unreadCount());
        Q_ASSERT(unreadCount() == 0);
    }
    */
    //
    emit changed();
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
    emit changed();
}

bool ForumGroup::hasChanged() const {
    return _hasChanged;
}

bool ForumGroup::isTemp() {
    return _temp;
}

void ForumGroup::emitChanged() {
    emit changed();
}

void ForumGroup::emitUnreadCountChanged() {
    emit unreadCountChanged();
}

void ForumGroup::incrementUnreadCount(const int &urc) {
    Q_ASSERT(isSubscribed() || urc==0);
    ForumDataItem::incrementUnreadCount(urc);
}

void ForumGroup::markToBeUpdated(bool toBe) {
    UpdateableItem::markToBeUpdated(toBe);
    if(toBe && subscription())
        subscription()->markToBeUpdated();
}

void ForumGroup::markRead(bool read) {
    for(ForumThread *ft : *this) {
        for(ForumMessage *msg : *ft) {
            msg->setRead(read);
        }
    }
}

QString ForumGroup::hierarchy() const {
    return _hierarchy;
}

QList<QObject *> ForumGroup::threads() const
{
    // todo: direct copy?
    QList<QObject*> myThreads;
    for(auto *thr : *this)
        myThreads.append(qobject_cast<QObject*>(thr));

    return myThreads;
}

void ForumGroup::setHierarchy(QString newHierarchy) {
    if (_hierarchy != newHierarchy) {
        _hierarchy = newHierarchy;
        _propertiesChanged = true;
        emitChanged();
    }
}
