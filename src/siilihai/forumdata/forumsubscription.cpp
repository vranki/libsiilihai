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
#include "forumsubscription.h"
#include "forumgroup.h"
#include "../parser/forumsubscriptionparsed.h"
#include "../tapatalk/forumsubscriptiontapatalk.h"

ForumSubscription::ForumSubscription(QObject *parent, bool temp, ForumProvider p) : QObject(parent),
    _provider(p) {
    _alias = QString::null;
    _latestThreads = 0;
    _latestMessages = 0;
    _authenticated = false;
    _unreadCount = 0;
    _username = _password = QString::null;
    _temp = temp;
    _groupListChanged = false;
    _beingUpdated = _beingSynced = _scheduledForUpdate = false;
    _forumId = 0;
    _engine = 0;
}

void ForumSubscription::copyFrom(ForumSubscription * other) {
    setForumId(other->forumId());
    setAlias(other->alias());
    setUsername(other->username());
    setPassword(other->password());
    setLatestThreads(other->latestThreads());
    setLatestMessages(other->latestMessages());
    setAuthenticated(other->authenticated());
}

ForumSubscription::~ForumSubscription() {
}

void ForumSubscription::addGroup(ForumGroup* grp, bool affectsSync, bool incrementUnreads) {
    Q_ASSERT(grp->subscription() == this);
    if(incrementUnreads) incrementUnreadCount(grp->unreadCount());
    insert(grp->id(), grp);
    if(affectsSync)
        setGroupListChanged();
    emit groupAdded(grp);
}

void ForumSubscription::removeGroup(ForumGroup* grp, bool affectsSync) {
    Q_ASSERT(grp->subscription() == this);
    remove(grp->id());

    if(affectsSync)
        setGroupListChanged();
    emit groupRemoved(grp);
    grp->deleteLater();
}

bool ForumSubscription::isSane() const {
    return (_alias.length() > 0 && _latestMessages > 0 && _latestThreads > 0);
}

QString ForumSubscription::toString() const {
    return QString("Subscription to ") + QString().number(_forumId) + " (" + _alias + ")";
}

QString ForumSubscription::alias() const {
    return _alias;
}

QString ForumSubscription::username() const {
    return _username;
}

QString ForumSubscription::password() const {
    return _password;
}

int ForumSubscription::latestThreads() const {
    return _latestThreads;
}

int ForumSubscription::latestMessages() const {
    return _latestMessages;
}

bool ForumSubscription::authenticated() const {
    return _authenticated;
}


void ForumSubscription::setAlias(QString name) {
    if(_alias==name) return;
    _alias = name;
    emit aliasChanged(_alias);
    emit changed();
}

void ForumSubscription::setUsername(QString username) {
    if(_username == username) return;
    _username = username;
    emit changed();
}
void ForumSubscription::setPassword(QString password) {
    if(_password==password) return;
    _password = password;
    emit changed();
}

void ForumSubscription::setLatestThreads(unsigned int lt) {
    if(lt==_latestThreads) return;
    _latestThreads = lt;
    emit changed();
}

void ForumSubscription::setLatestMessages(unsigned int lm) {
    if(lm==_latestMessages) return;
    _latestMessages = lm;
    emit changed();
}

void ForumSubscription::setAuthenticated(bool na) {
    if(na==_authenticated) return;
    _authenticated = na;
    emit changed();
}

int ForumSubscription::unreadCount() const {
    return _unreadCount;
}

void ForumSubscription::incrementUnreadCount(int urc) {
    if(!urc) return;
    _unreadCount += urc;
    Q_ASSERT(_unreadCount >= 0);
    emit unreadCountChanged();
}

bool ForumSubscription::isTemp() const {
    return _temp;
}

UpdateEngine *ForumSubscription::updateEngine() const {
    return _engine;
}

bool ForumSubscription::hasGroupListChanged() const {
    return _groupListChanged;
}

void ForumSubscription::setGroupListChanged(bool changed) {
    _groupListChanged = changed;
}

int ForumSubscription::forumId() const
{
    return _forumId;
}

void ForumSubscription::setForumId(int newId)
{
    _forumId = newId;
}

void ForumSubscription::markRead(bool read) {
    foreach(ForumGroup *group, values()) {
        group->markRead(read);
    }
}
void ForumSubscription::setBeingUpdated(bool bu) {
    Q_ASSERT(!_beingSynced);
    Q_ASSERT(!_scheduledForUpdate);
    _beingUpdated = bu;
    emit changed();
}

void ForumSubscription::setBeingSynced(bool bs) {
    Q_ASSERT(!_beingUpdated);
    _beingSynced = bs;
    emit changed();
}

void ForumSubscription::setScheduledForUpdate(bool su) {
    Q_ASSERT(!_beingUpdated);
    _scheduledForUpdate = su;
    emit changed();
}

bool ForumSubscription::beingUpdated() const {
    return _beingUpdated;
}

bool ForumSubscription::beingSynced() const {
    return _beingSynced;
}

bool ForumSubscription::scheduledForUpdate() const {
    return _scheduledForUpdate;
}

ForumSubscription::ForumProvider ForumSubscription::provider() const
{
    return _provider;
}

bool ForumSubscription::isParsed() const
{
    return _provider == FP_PARSER;
}

bool ForumSubscription::isTapaTalk() const
{
    return _provider == FP_TAPATALK;
}

ForumSubscription *ForumSubscription::newForProvider(ForumSubscription::ForumProvider fp, QObject *parent, bool temp) {
    if(fp==ForumSubscription::FP_PARSER)
        return new ForumSubscriptionParsed(parent, temp);
    if(fp==ForumSubscription::FP_TAPATALK)
        return new ForumSubscriptionTapaTalk(parent, temp);
    Q_ASSERT(false);
    return 0;
}
