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
#include <QObject>
#include <QQueue>
#include <QNetworkAccessManager>
#include "forumsession.h"
#include "forumparser.h"

class ForumSubscription;
class ForumGroup;
class ForumThread;
class ForumMessage;
class ForumDatabase;
class ParserManager;

/**
  * Handles updating a forum's data (threads, messages, etc) using a
  * ForumParser. Uses ForumSession to do low-level things. Stores
  * changes directly to a ForumDatabase.
  *
  * PES_MISSING_PARSER -> UPDATING_PARSER <-> IDLE <-> UPDATING
  *                                          -> ERROR -^
  * @see ForumDatabase
  * @see ForumSession
  * @see ForumParser
  */
class ParserEngine : public QObject {
    Q_OBJECT

public:
    enum ParserEngineState {
        PES_UNKNOWN=0,
        PES_MISSING_PARSER,
        PES_IDLE,
        PES_UPDATING,
        PES_ERROR,
        PES_UPDATING_PARSER
    };

    ParserEngine(ForumDatabase *fd, QObject *parent, ParserManager *pm);
    virtual ~ParserEngine();
    void setParser(ForumParser *fp);
    void setSubscription(ForumSubscription *fs);
    void updateGroupList();
    void updateForum(bool force=false);
    void updateThread(ForumThread *thread, bool force=false);
    ForumParser *parser();
    ParserEngine::ParserEngineState state();
    ForumSubscription* subscription();
    QNetworkAccessManager *networkAccessManager();
public slots:
    void cancelOperation();

signals:
    // Emitted if initially group list was empty but new groups were found.
    void groupListChanged(ForumSubscription *forum);
    void forumUpdated(ForumSubscription *forum);
    void statusChanged(ForumSubscription *forum, float progress);
    void updateFailure(ForumSubscription *forum, QString message);
    void getAuthentication(ForumSubscription *fsub, QAuthenticator *authenticator);
    void loginFinished(ForumSubscription *sub, bool success);
    void stateChanged(ParserEngine *engine, ParserEngine::ParserEngineState newState);

private slots:
    void listMessagesFinished(QList<ForumMessage*> &messages, ForumThread *thread, bool moreAvailable);
    void listGroupsFinished(QList<ForumGroup*> &groups);
    void listThreadsFinished(QList<ForumThread*> &threads, ForumGroup *group);
    void networkFailure(QString message);
    void loginFinishedSlot(ForumSubscription *sub, bool success);
    void subscriptionDeleted();
    void parserUpdated(ForumParser *p);
private:
    void updateNextChangedGroup();
    void updateNextChangedThread();
    void setBusy(bool busy);
    void updateCurrentProgress();
    void setState(ParserEngineState newState);
    ForumParser *currentParser;
    ForumSubscription *fsubscription;
    QNetworkAccessManager nam;
    ForumSession session;
    bool sessionInitialized;
    bool updateAll;
    bool forceUpdate; // Update even if no changes
    ForumDatabase *fdb;
    QQueue<ForumGroup*> groupsToUpdateQueue;
    QQueue<ForumThread*> threadsToUpdateQueue;
    ParserEngineState currentState;
    ParserManager *parserManager;
};

#endif /* PARSERENGINE_H_ */
