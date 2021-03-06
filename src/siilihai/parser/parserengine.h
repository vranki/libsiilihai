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
#include "../updateengine.h"
#include "forumparser.h"
#include "forumsubscriptionparsed.h"
#include <QTimer>
#include <QTextCodec>

class ParserManager;
class PatternMatcher;

#define FORUMID_ATTRIBUTE (QNetworkRequest::Attribute(QNetworkRequest::User + 1))

/**
  * Handles updating a forum's data (threads, messages, etc) using a
  * ForumParser.
  *
  * @see UpdateEngine
  * @see ForumDatabase
  * @see ForumParser
  *
  * @todo: Pool networkrequests so that they can be canceled properly!
  */
class ParserEngine : public UpdateEngine {
    Q_OBJECT

public:
    ParserEngine(QObject *parent=0, ForumDatabase *fd=0, ParserManager *pm=0, QNetworkAccessManager *n=0);
    virtual ~ParserEngine();
    // Sets the parser to be used. Ownership does not change.
    void setParser(ForumParser *fp);
    ForumParser *parser() const;
    // Sets the forumsubsctiption to be used. Ownership does not change. Set to null for no subscription.
    virtual void setSubscription(ForumSubscription *fs);

    void loginToForum();
    QString getMessageUrl(const ForumMessage *msg);
    QString getLoginUrl();
    QString getThreadListUrl(const ForumGroup *grp, int page=-1);
    QString getMessageListUrl(const ForumThread *thread, int page=-1);

    // Used by parsermaker
    void setPatternMatcher(PatternMatcher *newPm);
    void setGroup(ForumGroup *g);
    void setThread(ForumThread *t);
    //

    void performListGroups(QString &html);
    void performListThreads(QString &html);
    void performListMessages(QString &html);

    virtual void probeUrl(QUrl url);
    virtual QString engineTypeName();

    qint64 parserAge() const; // Age in days (or 999 if invalid)
public slots:
    virtual void cancelOperation();
    virtual void credentialsEntered(CredentialsRequest* cr);

protected:
    virtual void doUpdateForum();
    virtual void doUpdateGroup(ForumGroup *group);
    virtual void doUpdateThread(ForumThread *thread);
    virtual void resetState();
private slots:
    void networkReply(QNetworkReply *reply);
    void authenticationRequired (QNetworkReply * reply, QAuthenticator * authenticator); // Called by NAM
    void parserUpdated(ForumParser *p);
    void updateParserIfError(UpdateEngine *engine, UpdateEngine::UpdateEngineState newState, UpdateEngine::UpdateEngineState oldState);
    void cookieExpired(); // Called when cookie needs to be fetched again

signals:
    void listGroupsFinished(QList<ForumGroup*> &groups, ForumSubscription *sub);
    void listThreadsFinished(QList<ForumThread*> &foundThreads, ForumGroup *group); // Will be deleted
    void listMessagesFinished(QList<ForumMessage*> &foundMessages, ForumThread *thread, bool moreAvailable);
    void loginFinished(ForumSubscription *sub, bool success);
    void receivedHtml(const QString &data);

protected:
    virtual void requestCredentials();

private:
    enum ParserEngineOperation { PEONoOp=1, PEOLogin, PEOFetchCookie, PEOUpdateForum, PEOUpdateGroup, PEOUpdateThread, PEOEndList };
    bool parserMakerMode(); // Returns true if running in parser maker - less strict sanity tests
    void performLogin(QString &html);
    // Returns the subscription as ForumSubscriptionParsed instance
    ForumSubscriptionParsed *subscriptionParsed() const;
    ForumParser *currentParser;
    ParserManager *parserManager;
    bool updatingParser;

    void listGroupsReply(QNetworkReply *reply);
    void listThreadsReply(QNetworkReply *reply);
    void listMessagesReply(QNetworkReply *reply);
    void fetchCookieReply(QNetworkReply *reply);
    void loginReply(QNetworkReply *reply);

    bool prepareForUse(); // get cookie & login if needed. Returns true if needs to do stuff first.
    void nextOperation();
    void fetchCookie();
    void listThreadsOnNextPage();
    void listMessagesOnNextPage();
    void setRequestAttributes(QNetworkRequest &req, ParserEngineOperation op);
    QString statusReport();
    PatternMatcher *patternMatcher;
    QNetworkAccessManager *nam;
    QByteArray emptyData, loginData;
    bool cookieFetched, loggedIn, loggingIn;
    ParserEngineOperation operationInProgress;
    int currentListPage;

    // @todo consider changing to QVectors
    QList<ForumThread*> foundThreads; // Threads in currentGroup
    QList<ForumMessage*> foundMessages; // Represents messages in thread listMessages

    bool moreMessagesAvailable; // True if thread would have more messages but limit stops search
    QString currentMessagesUrl;
    QTimer cookieExpiredTimer;
    bool waitingForAuthentication;
    bool waitingForCookie;
    QTextCodec *codec; // Text codec to use for reading html.
};

#endif /* PARSERENGINE_H_ */
