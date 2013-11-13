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

     // Used by parsermaker
    void setPatternMatcher(PatternMatcher *newPm);

    void listGroups();
    void listThreads(ForumGroup *group);
    void listMessages(ForumThread *thread);
    void loginToForum();
    QString getMessageUrl(const ForumMessage *msg);
    QString getLoginUrl();
    QString getThreadListUrl(const ForumGroup *grp, int page=-1);
    QString getMessageListUrl(const ForumThread *thread, int page=-1);
    void performListGroups(QString &html);
    void performListThreads(QString &html);
    void performListMessages(QString &html);

    virtual void doUpdateForum();
    virtual void doUpdateGroup(ForumGroup *group);
    virtual void doUpdateThread(ForumThread *thread);
public slots:
    virtual void cancelOperation();
    virtual void credentialsEntered(CredentialsRequest* cr);
private slots:
    void networkReply(QNetworkReply *reply);
    void authenticationRequired (QNetworkReply * reply, QAuthenticator * authenticator); // Called by NAM
    void parserUpdated(ForumParser *p);
    void updateParserIfError(UpdateEngine::UpdateEngineState newState, UpdateEngine::UpdateEngineState oldState);
    void cookieExpired(); // Called when cookie needs to be fetched again

signals:
    void listGroupsFinished(QList<ForumGroup*> &groups, ForumSubscription *sub);
    void listThreadsFinished(QList<ForumThread*> &foundThreads, ForumGroup *group); // Will be deleted
    void listMessagesFinished(QList<ForumMessage*> &foundMessages, ForumThread *thread, bool moreAvailable);
    void loginFinished(ForumSubscription *sub, bool success);
    void receivedHtml(const QString &data);
    // Asynchronous
    void getHttpAuthentication(ForumSubscription *fsub, QAuthenticator *authenticator);

protected:
    virtual void requestCredentials();
private:
    enum ParserEngineOperation { PEONoOp=1, PEOLogin, PEOFetchCookie, PEOUpdateForum, PEOUpdateGroup, PEOUpdateThread };
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

    bool prepareForUse(); // get cookie & login if needed
    void nextOperation();
    void fetchCookie();
    void listThreadsOnNextPage();
    void listMessagesOnNextPage();
    void setRequestAttributes(QNetworkRequest &req, ParserEngineOperation op);

    QString convertCharset(const QByteArray &src);
    QString statusReport();
    PatternMatcher *patternMatcher;
    QNetworkAccessManager *nam;
    QByteArray emptyData, loginData;
    bool cookieFetched, loggedIn, loggingIn;
    ParserEngineOperation operationInProgress;
    QNetworkCookieJar *cookieJar;
    int currentListPage;

    // @todo consider changing to QVectors
    QList<ForumThread*> foundThreads; // Threads in currentGroup
    QList<ForumMessage*> foundMessages; // Represents messages in thread listMessages

    bool moreMessagesAvailable; // True if thread would have more messages but limit stops search
    QString currentMessagesUrl;
    QTimer cookieExpiredTimer;
    bool waitingForAuthentication;
};

#endif /* PARSERENGINE_H_ */
