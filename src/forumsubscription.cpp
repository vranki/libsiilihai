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

ForumSubscription::ForumSubscription(QObject *parent, bool temp) : QObject(parent) {
    _parser = -1;
    _alias = QString::null;
    _latestThreads = 0;
    _latestMessages = 0;
    _authenticated = false;
    _unreadCount = 0;
    _username = _password = QString::null;
    _temp = temp;
    _engine = 0;
    _groupListChanged = false;
}

void ForumSubscription::copyFrom(ForumSubscription * other) {
    setParser(other->parser());
    setAlias(other->alias());
    setUsername(other->username());
    setPassword(other->password());
    setLatestThreads(other->latestThreads());
    setLatestMessages(other->latestMessages());
    setAuthenticated(other->authenticated());
}

ForumSubscription::~ForumSubscription() {
}

void ForumSubscription::addGroup(ForumGroup* grp, bool affectsSync) {
    Q_ASSERT(grp->subscription() == this);
    incrementUnreadCount(grp->unreadCount());
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
}

bool ForumSubscription::isSane() const {
    return (_parser > 0 && _alias.length() > 0 && _latestMessages > 0 && _latestThreads > 0);
}

QString ForumSubscription::toString() const {
    return QString("Subscription to ") + QString().number(_parser) + " (" + _alias + ")";
}

int ForumSubscription::parser() const {
    return _parser;
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

void ForumSubscription::setParser(int parser) {
    if(parser==_parser) return;
    _parser = parser;
    emit changed(this);
}

void ForumSubscription::setAlias(QString name) {
    if(_alias==name) return;
    _alias = name;
    emit changed(this);
}

void ForumSubscription::setUsername(QString username) {
    if(_username == username) return;
    _username = username;
    emit changed(this);
}
void ForumSubscription::setPassword(QString password) {
    if(_password==password) return;
    _password = password;
    emit changed(this);
}

void ForumSubscription::setLatestThreads(unsigned int lt) {
    if(lt==_latestThreads) return;
    _latestThreads = lt;
    emit changed(this);
}

void ForumSubscription::setLatestMessages(unsigned int lm) {
    if(lm==_latestMessages) return;
    _latestMessages = lm;
    emit changed(this);
}

void ForumSubscription::setAuthenticated(bool na) {
    if(na==_authenticated) return;
    _authenticated = na;
    emit changed(this);
}

int ForumSubscription::unreadCount() const {
    return _unreadCount;
}

void ForumSubscription::incrementUnreadCount(int urc) {
    if(!urc) return;
    _unreadCount += urc;
    Q_ASSERT(_unreadCount >= 0);
    emit unreadCountChanged(this);
}

bool ForumSubscription::isTemp() const {
    return _temp;
}

void ForumSubscription::setParserEngine(ParserEngine *eng) {
    _engine = eng;
    emit changed(this);
}

ParserEngine *ForumSubscription::parserEngine() const {
    return _engine;
}

bool ForumSubscription::hasGroupListChanged() const {
    return _groupListChanged;
}

void ForumSubscription::setGroupListChanged(bool changed) {
    _groupListChanged = changed;
}

void ForumSubscription::markRead(bool read) {
    foreach(ForumGroup *group, values()) {
        group->markRead(read);
    }
}
