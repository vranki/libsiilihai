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
#ifndef TAPATALKFORUMSUBSCRIPTION_H
#define TAPATALKFORUMSUBSCRIPTION_H
#include "../forumdata/forumsubscription.h"
#include <QUrl>

class TapaTalkEngine;
#define SUB_FORUMURL "forumurl"

class ForumSubscriptionTapaTalk : public ForumSubscription
{
    Q_OBJECT
public:
    explicit ForumSubscriptionTapaTalk(QObject *parent = 0, bool temp=true);
    void setTapaTalkEngine(TapaTalkEngine* newEngine);
    virtual QDomElement serialize(QDomElement &parent, QDomDocument &doc);
};

#endif // TAPATALKFORUMSUBSCRIPTION_H
