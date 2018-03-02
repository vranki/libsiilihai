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
#include "forumgroup.h"
#include "forummessage.h"
#include "forumsubscription.h"

ForumThread::~ForumThread() { }

ForumThread::ForumThread(QObject *parent, bool temp) : ForumDataItem(parent)
  , _ordernum(-1)
  , _changeset(-1)
  , _group(nullptr)
  , _hasMoreMessages(false)
  , _getMessagesCount(999)
  , _temp(temp)
  , _lastPage(0)
{
    connect(this, &ForumThread::messageRemoved,
            this, &ForumThread::messagesChanged);
    connect(this, &ForumThread::messageAdded,
            this, &ForumThread::messagesChanged);
}

void ForumThread::copyFrom(const ForumThread * o) {
    setId(o->id());
    setName(o->name());
    setLastChange(o->lastChange());
    setChangeset(o->changeset());
    setOrdernum(o->ordernum());
    setHasMoreMessages(o->hasMoreMessages());
    setGetMessagesCount(o->getMessagesCount());
    setLastPage(o->lastPage());
}

ForumMessage *ForumThread::value(const QString &id) const {
    for(ForumMessage *msg : *this) {
        if(msg->id() == id) return msg;
    }
    return nullptr;
}

bool ForumThread::contains(const QString &id) const {
    return value(id);
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
            tparser = QString().number(group()->subscription()->id());
    }
    return tparser + "/" + tgroup + "/" + id() + ": " + name();
}

bool ForumThread::isSane() const {
    return (id().length() > 0 && _getMessagesCount >= 0);
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

void ForumThread::setGroup(ForumGroup *grp) {
    Q_ASSERT(!grp || (grp->isTemp() == isTemp()));
    _group = grp;
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

bool ForumThread::isTemp() const {
    return _temp;
}

void ForumThread::emitChanged() {
    emit changed();
}

void ForumThread::emitUnreadCountChanged() {
    emit unreadCountChanged();
}

void ForumThread::updateMessagesObjectList()
{
    _messagesObjectList.clear();
    for(ForumMessage *msg : *this)
        _messagesObjectList.append(msg);
}

void ForumThread::setLastPage(int lp) {
    if(_lastPage == lp) return;
    _lastPage = lp;
    _propertiesChanged = true;
}

int ForumThread::lastPage() const {
    return _lastPage;
}

void ForumThread::addMessage(ForumMessage *msg, bool affectsSync) {
    Q_ASSERT(!msg->thread());
    Q_ASSERT(!contains(msg->id()));
    Q_ASSERT(msg->isTemp() == isTemp());
    msg->setThread(this);

    if(msg->isRead()) {
        if(affectsSync && group()) group()->setHasChanged(true);
    } else {
        incrementUnreadCount(1);
        if(group()) {
            group()->incrementUnreadCount(1);
            if(group()->isSubscribed())
                group()->subscription()->incrementUnreadCount(1);
        }
    }
    append(msg);
    // Sort:
    std::sort(begin(), end(), [](ForumMessage* a, ForumMessage* b) {
        return a->ordernum() < b->ordernum();
    });
#ifdef SANITY_CHECKS
    Q_ASSERT(unreadCount() >= 0);
    Q_ASSERT(unreadCount() <= size());
#endif
    _displayName = QString();
    updateMessagesObjectList();
    emit messageAdded(msg);
}

void ForumThread::removeMessage(ForumMessage *msg, bool affectsSync) {
    Q_ASSERT(msg->thread() == this);
    if(!msg->isRead() && group()->isSubscribed()) {
        incrementUnreadCount(-1);
        group()->incrementUnreadCount(-1);
        if(group()->isSubscribed())
            group()->subscription()->incrementUnreadCount(-1);
    }
    if(affectsSync && msg->isRead()) group()->setHasChanged(true);
    Q_ASSERT(contains(msg->id()));
    removeOne(msg);
    _displayName = QString();
    updateMessagesObjectList();
    emit messageRemoved(msg);
    msg->deleteLater();
}

bool ForumThread::needsToBeUpdated() const {
    return lastChange() == NEEDS_UPDATE || UpdateableItem::needsToBeUpdated();
}

void ForumThread::markToBeUpdated(bool toBe) {
    UpdateableItem::markToBeUpdated(toBe);
    if(toBe && group()) {
        group()->markToBeUpdated();
    }
}

QObjectList ForumThread::messages() const {
    return _messagesObjectList;
}
