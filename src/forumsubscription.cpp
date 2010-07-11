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

ForumSubscription::ForumSubscription() : QObject() {
	_parser = -1;
        _alias = QString::null;
	_latestThreads = 0;
	_latestMessages = 0;
        _authenticated = false;
    }

ForumSubscription::ForumSubscription(QObject *parent) : QObject(parent) {
	ForumSubscription();
}

ForumSubscription& ForumSubscription::operator=(const ForumSubscription& other) {
	_parser = other._parser;
        _alias = other._alias;
	_username = other._username;
	_password = other._password;
	_latestThreads = other._latestThreads;
	_latestMessages = other._latestMessages;
        _authenticated = other._authenticated;
	return *this;
}

ForumSubscription::ForumSubscription(const ForumSubscription& other) : QObject() {
	*this = other;
}

ForumSubscription::~ForumSubscription() {
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
unsigned int ForumSubscription::latest_threads() const {
	return _latestThreads;
}
unsigned int ForumSubscription::latest_messages() const {
	return _latestMessages;
}

bool ForumSubscription::authenticated() const {
    return _authenticated;
}
void ForumSubscription::setParser(int parser) {
	_parser = parser;
}
void ForumSubscription::setAlias(QString name) {
        _alias = name;
}
void ForumSubscription::setUsername(QString username) {
	_username = username;
}
void ForumSubscription::setPassword(QString password) {
	_password = password;
}
void ForumSubscription::setLatestThreads(unsigned int lt) {
	_latestThreads = lt;
}
void ForumSubscription::setLatestMessages(unsigned int lm) {
	_latestMessages = lm;
}

void ForumSubscription::setAuthenticated(bool na) {
    _authenticated = na;
}
