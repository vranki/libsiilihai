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
#include "../xmlserialization.h"

ForumSubscription::ForumSubscription(QObject *parent, bool temp, ForumProvider p) : QObject(parent) {
    _provider = p;
    _alias = QString::null;
    _latestThreads = 0;
    _latestMessages = 0;
    _supportsLogin = _authenticated = false;
    _unreadCount = 0;
    _username = _password = QString::null;
    _temp = temp;
    _groupListChanged = false;
    _beingUpdated = _beingSynced = _scheduledForUpdate = _scheduledForSync = false;
    _forumId = 0;
    _engine = 0;
}

void ForumSubscription::copyFrom(ForumSubscription * other) {
    setId(other->id());
    setAlias(other->alias());
    setUsername(other->username());
    setPassword(other->password());
    setLatestThreads(other->latestThreads());
    setLatestMessages(other->latestMessages());
    setAuthenticated(other->isAuthenticated());
    setSupportsLogin(other->supportsLogin());
    setForumUrl(other->forumUrl());
}

ForumSubscription::~ForumSubscription() {
}

void ForumSubscription::addGroup(ForumGroup* grp, bool affectsSync, bool incrementUnreads) {
    Q_ASSERT(!grp->subscription());
    grp->setSubscription(this);
    if(incrementUnreads) incrementUnreadCount(grp->unreadCount());
    insert(grp->id(), grp);
    if(affectsSync)
        setGroupListChanged();
    emit groupAdded(grp);
}

void ForumSubscription::removeGroup(ForumGroup* grp, bool affectsSync, bool incrementUnreads) {
    Q_ASSERT(grp->subscription() == this);
    if(incrementUnreads) incrementUnreadCount(-grp->unreadCount());
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
    return QString("Subscription to %1 (%2) su %3 bu %4 ss %5 bs %6").arg(_forumId).arg(_alias).arg(_scheduledForUpdate)
            .arg(_beingUpdated).arg(_scheduledForSync).arg(_beingSynced);
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

bool ForumSubscription::isAuthenticated() const {
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

void ForumSubscription::engineDestroyed() {
    _engine = 0;
}

bool ForumSubscription::hasGroupListChanged() const {
    return _groupListChanged;
}

void ForumSubscription::setGroupListChanged(bool changed) {
    _groupListChanged = changed;
}

int ForumSubscription::id() const
{
    return _forumId;
}

void ForumSubscription::setId(int newId)
{
    _forumId = newId;
    emit changed();
}

void ForumSubscription::markRead(bool read) {
    foreach(ForumGroup *group, values()) {
        group->markRead(read);
    }
}

void ForumSubscription::setBeingUpdated(bool bu) {
    Q_ASSERT(!(_beingSynced && bu));
    Q_ASSERT(!(_scheduledForUpdate && bu));
    _beingUpdated = bu;
    emit changed();
}

void ForumSubscription::setBeingSynced(bool bs) {
    Q_ASSERT(!(_beingUpdated && bs));
    _beingSynced = bs;
    emit changed();
}

void ForumSubscription::setScheduledForUpdate(bool su) {
    Q_ASSERT(!(_beingUpdated && su));
    Q_ASSERT(!(_scheduledForSync && su));
    _scheduledForUpdate = su;
    emit changed();
}

void ForumSubscription::setScheduledForSync(bool su)
{
    Q_ASSERT(!(_beingSynced && su));
    Q_ASSERT(!(_beingUpdated && su));
    _scheduledForSync = su;
    emit changed();
}

bool ForumSubscription::beingUpdated() const {
    return _beingUpdated;
}

bool ForumSubscription::beingSynced() const {
    return _beingSynced;
}

bool ForumSubscription::scheduledForSync() const
{
    return _scheduledForSync;
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

QDomElement ForumSubscription::serialize(QDomElement &parent, QDomDocument &doc) {
    QDomElement subElement = doc.createElement(SUB_SUBSCRIPTION);

    XmlSerialization::appendValue(SUB_FORUMID, QString::number(id()), subElement, doc);
    XmlSerialization::appendValue(SUB_PROVIDER, QString::number(provider()), subElement, doc);
    XmlSerialization::appendValue(SUB_ALIAS, alias(), subElement, doc);
    XmlSerialization::appendValue(SUB_USERNAME, username(), subElement, doc);
    XmlSerialization::appendValue(SUB_PASSWORD, password(), subElement, doc);
    XmlSerialization::appendValue(SUB_LATEST_THREADS, QString::number(latestThreads()), subElement, doc);
    XmlSerialization::appendValue(SUB_LATEST_MESSAGES, QString::number(latestMessages()), subElement, doc);

    parent.appendChild(subElement);

    return subElement;
}

ForumSubscription* ForumSubscription::readSubscription(QDomElement &element, QObject *parent) {
    ForumSubscription *sub = 0;
    if(element.tagName() != SUB_SUBSCRIPTION) return 0;
    bool ok = false;
    int provider = QString(element.firstChildElement(SUB_PROVIDER).text()).toInt(&ok);
    if(!ok || provider <=0) return 0;
    sub = ForumSubscription::newForProvider((ForumSubscription::ForumProvider) provider, parent, false);
    if(!sub) return 0;
    sub->readSubscriptionXml(element);
    return sub;
}

void ForumSubscription::readSubscriptionXml(QDomElement &element)
{
    setId(QString(element.firstChildElement(SUB_FORUMID).text()).toInt());
    setAlias(element.firstChildElement(SUB_ALIAS).text());
    setUsername(element.firstChildElement(SUB_USERNAME).text());
    setPassword(element.firstChildElement(SUB_PASSWORD).text());
    setLatestThreads(QString(element.firstChildElement(SUB_LATEST_THREADS).text()).toInt());
    setLatestMessages(QString(element.firstChildElement(SUB_LATEST_MESSAGES).text()).toInt());
    setAuthenticated(username().length() > 0);
}

void ForumSubscription::setForumUrl(QUrl url)
{
    _forumUrl = url;
    emit changed();
}

void ForumSubscription::setSupportsLogin(bool sl)
{
    _supportsLogin = sl;
    emit changed();
}

bool ForumSubscription::supportsLogin() const {
    return _supportsLogin;
}

void ForumSubscription::setSupportsPosting(bool sp) {
    _supportsPosting = sp;
    emit changed();
}

bool ForumSubscription::supportsPosting() const {
    return _supportsPosting;
}

QString ForumSubscription::faviconUrl()
{
    QString fiUrl = forumUrl().toString(); // @todo modify to QUrl handling..
    if(fiUrl.isNull()) return fiUrl;
    QString path = QUrl(fiUrl).path();
    if(path.length() > 1)
        fiUrl = fiUrl.replace(path, "");
    fiUrl = fiUrl + "/favicon.ico";
    return fiUrl;
}

void ForumSubscription::setProvider(ForumSubscription::ForumProvider provider)
{
    _provider = provider;
    emit changed();
}

QUrl ForumSubscription::forumUrl() const
{
    return _forumUrl;
}
