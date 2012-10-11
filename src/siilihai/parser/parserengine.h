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
#ifndef PARSERENGINE_H_
#define PARSERENGINE_H_
#include "../forumsession.h"
#include "../updateengine.h"
#include "forumparser.h"
#include "forumsubscriptionparsed.h"

class ParserManager;

/**
  * Handles updating a forum's data (threads, messages, etc) using a
  * ForumParser. Uses ForumSession to do low-level things. Stores
  * changes directly to a ForumDatabase.
  *
  * PES_MISSING_PARSER -> UPDATING_PARSER <-> REQUESTING_CREDENTIALS <-> IDLE <-> UPDATING
  *                                    '-->----------------------->------^ ^- ERROR <-'
  * @see ForumDatabase
  * @see ForumSession
  * @see ForumParser
  */
class ParserEngine : public UpdateEngine {
    Q_OBJECT

public:
    ParserEngine(ForumDatabase *fd, QObject *parent, ParserManager *pm);
    virtual ~ParserEngine();
    void setParser(ForumParser *fp);

    virtual void setSubscription(ForumSubscription *fs);
    virtual void updateThread(ForumThread *thread, bool force=false);
    ForumParser *parser() const;
public slots:
    virtual void cancelOperation();
    virtual void credentialsEntered(CredentialsRequest* cr);

signals:

private slots:
    void parserUpdated(ForumParser *p);
protected:
    virtual void requestCredentials();
    virtual void doUpdateForum();
    virtual void doUpdateGroup(ForumGroup *group);
    virtual void doUpdateThread(ForumThread *thread);
private:
    void initSession();
    ForumSubscriptionParsed *subscriptionParsed() const;
    ForumParser *currentParser;
    ForumSession session;
    bool sessionInitialized;
    ParserManager *parserManager;
    bool updatingParser;
};

#endif /* PARSERENGINE_H_ */
