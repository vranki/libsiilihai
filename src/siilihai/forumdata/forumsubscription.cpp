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
#include <QUrl>
#include "forumsubscription.h"
#include "forumgroup.h"
#include "../parser/forumsubscriptionparsed.h"
#include "../tapatalk/forumsubscriptiontapatalk.h"
#include "../discourse/forumsubscriptiondiscourse.h"
#include "../xmlserialization.h"

const QString providerNames[] = {"None", "Parser", "TapaTalk", "Discourse"};

ForumSubscription::ForumSubscription(QObject *parent, bool temp, ForumProvider p)
    : QObject(parent)
    , _engine(nullptr)
    , _forumId(0)
    , _latestThreads(20)
    , _latestMessages(20)
    , _authenticated(false)
    , _supportsLogin(false)
    , _supportsPosting(false)
    , _unreadCount(0)
    , _temp(temp)
    , _groupListChanged(false)
    , _scheduledForUpdate(false)
    , _beingUpdated(false)
    , _scheduledForSync(false)
    , _beingSynced(false)
    , _provider(p)
{
    connect(this, SIGNAL(groupAdded(ForumGroup*)), this, SIGNAL(groupsChanged()));
    connect(this, SIGNAL(groupRemoved(ForumGroup*)), this, SIGNAL(groupsChanged()));
}

void ForumSubscription::copyFrom(const ForumSubscription * other) {
    setId(other->id());
    setAlias(other->alias());
    setUsername(other->username());
    setPassword(other->password());
    setLatestThreads(other->latestThreads());
    setLatestMessages(other->latestMessages());
    setAuthenticated(other->isAuthenticated());
    setSupportsLogin(other->supportsLogin());
    setForumUrl(other->forumUrl());
    setProvider(other->provider());
}

ForumSubscription::~ForumSubscription() {
    clearErrors();
}

void ForumSubscription::addGroup(ForumGroup *grp, bool affectsSync, bool incrementUnreads) {
    Q_ASSERT(!grp->subscription());
    Q_ASSERT(!contains(grp->id()));
    Q_ASSERT(grp->isTemp() == isTemp());
    grp->setSubscription(this);
    if(incrementUnreads) incrementUnreadCount(grp->unreadCount());
    append(grp);
    if(affectsSync) setGroupListChanged();
    emit groupAdded(grp);
}

void ForumSubscription::removeGroup(ForumGroup* grp, bool affectsSync, bool incrementUnreads) {
    Q_ASSERT(grp->subscription() == this);
    Q_ASSERT(contains(grp->id()));
    if(incrementUnreads) incrementUnreadCount(-grp->unreadCount());
    removeOne(grp);

    if(affectsSync) setGroupListChanged();
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

ForumGroup *ForumSubscription::value(const QString &id) const
{
    for(ForumGroup *grp : *this) {
        if(grp->id() == id) return grp;
    }
    return nullptr;
}

bool ForumSubscription::contains(const QString &id) const
{
    return value(id);
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
    _engine = nullptr;
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
    for(auto group : *this)
        group->markRead(read);
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

QString ForumSubscription::providerName() const
{
    return providerNames[provider()];
}

ForumSubscription *ForumSubscription::newForProvider(ForumSubscription::ForumProvider fp, QObject *parent, bool temp) {
    if(fp==ForumSubscription::FP_PARSER)
        return new ForumSubscriptionParsed(parent, temp);
    if(fp==ForumSubscription::FP_TAPATALK)
        return new ForumSubscriptionTapaTalk(parent, temp);
    if(fp==ForumSubscription::FP_DISCOURSE)
        return new ForumSubscriptionDiscourse(parent, temp);
    Q_ASSERT(false);
    return nullptr;
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
    if(_provider != FP_PARSER) {
        XmlSerialization::appendValue(SUB_FORUMURL, forumUrl().toString(), subElement, doc);
    }
    parent.appendChild(subElement);

    return subElement;
}

ForumSubscription* ForumSubscription::readSubscription(QDomElement &element, QObject *parent) {
    ForumSubscription *sub = nullptr;
    if(element.tagName() != SUB_SUBSCRIPTION) return nullptr;
    bool ok = false;
    int provider = QString(element.firstChildElement(SUB_PROVIDER).text()).toInt(&ok);
    if(!ok || provider <=0) return nullptr;
    sub = ForumSubscription::newForProvider(static_cast<ForumSubscription::ForumProvider> (provider), parent, false);
    if(!sub) return nullptr;
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
    if(_provider != FP_PARSER) {
        QUrl forumUrl = QUrl(element.firstChildElement(SUB_FORUMURL).text());
        setForumUrl(forumUrl);
    }
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

QUrl ForumSubscription::faviconUrl() const
{
    QUrl fiUrl = forumUrl();
    fiUrl.setPath("/favicon.ico");
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


// Error list related
QList<UpdateError *> ForumSubscription::errorList()
{
    return m_errors;
}
void ForumSubscription::appendError(UpdateError *ue)
{
    m_errors.append(ue);
    emit errorsChanged();
}

void ForumSubscription::clearErrors()
{
    for(auto error : m_errors) error->deleteLater();
    m_errors.clear();
    emit errorsChanged();
}

QQmlListProperty<UpdateError> ForumSubscription::errors()
{
    return QQmlListProperty<UpdateError>(this, 0, &ForumSubscription::append_error, &ForumSubscription::count_error, &ForumSubscription::at_error, &ForumSubscription::clear_error);
}

QList<QObject *> ForumSubscription::groups() const
{
    QList<QObject*> myGroups;

    for(auto *grp : *this)
        myGroups.append(qobject_cast<QObject*>(grp));

    return myGroups;
}

QList<QObject *> ForumSubscription::subscribedGroups() const
{
    QList<QObject*> myGroups;

    for(auto *grp : *this) {
        if(grp->isSubscribed()) myGroups.append(qobject_cast<QObject*>(grp));
    }

    return myGroups;
}

void ForumSubscription::append_error(QQmlListProperty<UpdateError> *list, UpdateError *msg)
{
    ForumSubscription *sub = qobject_cast<ForumSubscription *>(list->object);
    if (msg) {
        sub->m_errors.append(msg);
    }
}

int ForumSubscription::count_error(QQmlListProperty<UpdateError> *list)
{
    ForumSubscription *sub = qobject_cast<ForumSubscription *>(list->object);
    return sub->m_errors.size();
}

UpdateError* ForumSubscription::at_error(QQmlListProperty<UpdateError> *list, int index)
{
    ForumSubscription *sub = qobject_cast<ForumSubscription *>(list->object);
    return sub->m_errors.at(index);
}

void ForumSubscription::clear_error(QQmlListProperty<UpdateError> *list)
{
    ForumSubscription *sub = qobject_cast<ForumSubscription *>(list->object);
    sub->clearErrors();
}
