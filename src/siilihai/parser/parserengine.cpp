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
#include "parserengine.h"
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QStringList>
#include "../forumdata/forumgroup.h"
#include "../forumdata/forumthread.h"
#include "../forumdata/forummessage.h"
#include "../forumdatabase/forumdatabase.h"
#include "../forumdata/forumsubscription.h"
#include "../credentialsrequest.h"
#include "patternmatcher.h"
#include "parsermanager.h"
#include "../httppost.h"

static const QString operationNames[] = {"(null)", "NoOp", "Login", "FetchCookie", "ListGroups", "ListThreads", "ListMessages" };

ParserEngine::ParserEngine(QObject *parent, ForumDatabase *fd, ParserManager *pm, QNetworkAccessManager *n) :
    UpdateEngine(parent, fd), parserManager(pm), nam(n) {

    connect(this, SIGNAL(listGroupsFinished(QList<ForumGroup*>&, ForumSubscription *)), this, SLOT(listGroupsFinished(QList<ForumGroup*>&, ForumSubscription *)));
    connect(this, SIGNAL(listThreadsFinished(QList<ForumThread*>&, ForumGroup*)), this, SLOT(listThreadsFinished(QList<ForumThread*>&, ForumGroup*)));
    connect(this, SIGNAL(listMessagesFinished(QList<ForumMessage*>&, ForumThread*, bool)),
            this, SLOT(listMessagesFinished(QList<ForumMessage*>&, ForumThread*, bool)));
    connect(this, SIGNAL(networkFailure(QString)), this, SLOT(networkFailure(QString)));
    connect(this, SIGNAL(loginFinished(ForumSubscription *,bool)), this, SLOT(loginFinishedSlot(ForumSubscription *,bool)));
    connect(this, SIGNAL(stateChanged(UpdateEngine::UpdateEngineState,UpdateEngine::UpdateEngineState)),
            this, SLOT(updateParserIfError(UpdateEngine::UpdateEngineState,UpdateEngine::UpdateEngineState)));

    if(parserManager)
        connect(parserManager, SIGNAL(parserUpdated(ForumParser*)), this, SLOT(parserUpdated(ForumParser*)));

    if(!nam)
        nam = new QNetworkAccessManager(this);
    nam->setProxy(QNetworkProxy::applicationProxy());
    connect(nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkReply(QNetworkReply*)), Qt::UniqueConnection);

    currentParser = 0;
    updatingParser = false;
    operationInProgress = PEONoOp;
    cookieJar = 0;
    currentListPage = 0;
    setPatternMatcher(new PatternMatcher(this));
    loggedIn = loggingIn = false;
    cookieFetched = false;
    cookieExpiredTimer.setSingleShot(true);
    cookieExpiredTimer.setInterval(10*60*1000);
    waitingForAuthentication = false;
    connect(&cookieExpiredTimer, SIGNAL(timeout()), this, SLOT(cookieExpired()));
    connect(nam, SIGNAL(authenticationRequired(QNetworkReply *, QAuthenticator *)),
            this, SLOT(authenticationRequired(QNetworkReply *, QAuthenticator *)), Qt::UniqueConnection);
}

ParserEngine::~ParserEngine() {
}

void ParserEngine::setParser(ForumParser *fp) {
    currentParser = fp;
    if(fp) {
        if(subscription() && !updatingParser && state()==UES_ENGINE_NOT_READY)  {
            setState(UES_IDLE); // We have both parser & sub
        }
    } else { // No parser
        if(!updatingParser)
            setState(UES_ENGINE_NOT_READY);
    }
}

void ParserEngine::setSubscription(ForumSubscription *fs) {
    Q_ASSERT(!fs || qobject_cast<ForumSubscriptionParsed*>(fs));
    UpdateEngine::setSubscription(fs);
    if(!fs) return;
    subscriptionParsed()->setParserEngine(this);

    bool updateParser = false;

    if(!currentParser) {
        updateParser = true;
    } else {
        QDate updateDate = QDate::currentDate();
        updateDate.addDays(-2); // Update if older than 2 days

        if(currentParser->update_date < updateDate) updateParser = true;
    }
    if(updateParser && parserManager) {
        parserManager->updateParser(subscriptionParsed()->parserId());
        updatingParser = true;
        setState(UES_ENGINE_NOT_READY);
    } else {
        setState(UES_IDLE);
    }
}

void ParserEngine::setPatternMatcher(PatternMatcher *newPm) {
    patternMatcher = newPm;
}

ForumParser *ParserEngine::parser() const {
    return currentParser;
}

void ParserEngine::parserUpdated(ForumParser *p) {
    Q_ASSERT(p);
    if(subscriptionParsed()->parserId() == p->id()) {
        updatingParser = false;
        setParser(p); // Changes to idle state if all is good
    }
}

void ParserEngine::updateParserIfError(UpdateEngine::UpdateEngineState newState, UpdateEngine::UpdateEngineState oldState) {
    if(newState == UES_ERROR && parserManager) {
        parserManager->updateParser(subscriptionParsed()->parserId());
    }
}

void ParserEngine::cancelOperation() {
    qDebug() << Q_FUNC_INFO << (subscription() ? subscription()->toString() : "(no sub)");

    updateAll = false;
    updateCanceled = true;
    if(operationInProgress == PEOUpdateForum) {
        QList<ForumGroup*> emptyList;
        emit listGroupsFinished(emptyList, subscription());
    }
    if(operationInProgress == PEOUpdateGroup) {
        QList<ForumThread*> emptyList;
        ForumGroup *updatedGroup = groupBeingUpdated;
        groupBeingUpdated = 0;
        emit listThreadsFinished(emptyList, updatedGroup);
    }

    if(operationInProgress == PEOUpdateThread) {
        QList<ForumMessage*> emptyList;
        ForumThread *updatedThread = threadBeingUpdated;
        threadBeingUpdated = 0;
        emit listMessagesFinished(emptyList, updatedThread, false);
    }

    if(operationInProgress == PEOLogin) emit loginFinished(subscription(), false);

    operationInProgress = PEONoOp;
    cookieFetched = false;
    currentListPage = -1;
    qDeleteAll(foundMessages);
    foundMessages.clear();
    qDeleteAll(foundThreads);
    foundThreads.clear();
    groupBeingUpdated = 0;
    threadBeingUpdated = 0;
    waitingForAuthentication = false;
    UpdateEngine::cancelOperation();
}

void ParserEngine::requestCredentials() {
    UpdateEngine::requestCredentials();
    if(parser()->login_type==ForumParser::LoginTypeHttpPost) {
        emit getForumAuthentication(subscription());
    } else if(parser()->login_type==ForumParser::LoginTypeHttpAuth) {
        emit getHttpAuthentication(subscription(), 0);
    }
}

bool ParserEngine::parserMakerMode() {
    return !parserManager; // @todo smarter way
}

void ParserEngine::credentialsEntered(CredentialsRequest* cr) {
    if(cr->credentialType==CredentialsRequest::SH_CREDENTIAL_HTTP) {
        Q_ASSERT(waitingForAuthentication);
        qDebug() << Q_FUNC_INFO;
        waitingForAuthentication = false;
    }
    UpdateEngine::credentialsEntered(cr);
}


// Avoid using asserts that can break parsermaker here! Put them elsewhere
void ParserEngine::doUpdateForum() {
    // Can be noop if cookie was fetched
    Q_ASSERT(operationInProgress == ParserEngine::PEONoOp || operationInProgress == ParserEngine::PEOUpdateForum);
    operationInProgress = ParserEngine::PEOUpdateForum;

    if(prepareForUse()) {
        qDebug() << Q_FUNC_INFO << "Need to fetch cookie etc first, NOT updating yet";
        return;
    }

    QNetworkRequest req(QUrl(parser()->forum_url));
    setRequestAttributes(req, PEOUpdateForum);
    nam->post(req, emptyData);
}

// Avoid using asserts that can break parsermaker here! Put them elsewhere
void ParserEngine::doUpdateGroup(ForumGroup *group) {
    Q_ASSERT(group->isSubscribed());
    Q_ASSERT(!threadBeingUpdated);
    Q_ASSERT(operationInProgress == ParserEngine::PEONoOp || operationInProgress == ParserEngine::PEOUpdateGroup);

    if (operationInProgress != PEONoOp && operationInProgress != PEOUpdateGroup) {
        qWarning() << Q_FUNC_INFO << "Operation " << operationNames[operationInProgress] << " in progress!! Don't command me yet!";
        return;
    }
    operationInProgress = PEOUpdateGroup;
    groupBeingUpdated = group;
    currentMessagesUrl = QString::null;
    if(prepareForUse()) return;
    currentListPage = parser()->thread_list_page_start;
    listThreadsOnNextPage();
}

// Avoid using asserts that can break parsermaker here! Put them elsewhere
// Remember, this can be called after fetchcookie sometimes!
void ParserEngine::doUpdateThread(ForumThread *thread)
{
    Q_ASSERT(!threadBeingUpdated || thread == threadBeingUpdated);
    // Q_ASSERT(!groupBeingUpdated); don't care
    Q_ASSERT(operationInProgress == ParserEngine::PEONoOp || operationInProgress == ParserEngine::PEOUpdateThread);
    Q_ASSERT(thread->isSane());

    if (operationInProgress != PEONoOp &&
            operationInProgress != PEOUpdateThread) { // This can be called from nextOperation, after fetchcookie
        qWarning() << Q_FUNC_INFO << "Operation " << operationNames[operationInProgress] << " in progress!! Don't command me yet!";
        Q_ASSERT(false);
        return;
    }
    operationInProgress = PEOUpdateThread;
    threadBeingUpdated = thread;
    Q_ASSERT(groupBeingUpdated == 0 || groupBeingUpdated == thread->group());
    groupBeingUpdated = thread->group();
    foundMessages.clear();
    moreMessagesAvailable = false;
    if(prepareForUse()) return;
    /* @todo enable this when it's figured out how to add the new messages to
      end of thread and NOT delete the messages in beginning of thread.

    if(thread->getLastPage()) { // Start from last known page if possible
        currentListPage = thread->getLastPage();
    } else {*/
    currentListPage = parser()->view_thread_page_start;
    //  }

    listMessagesOnNextPage();
}

void ParserEngine::networkReply(QNetworkReply *reply) {
    int operationAttribute = reply->request().attribute(QNetworkRequest::User).toInt();
    int forumId = reply->request().attribute(FORUMID_ATTRIBUTE).toInt();
    if(forumId != subscription()->forumId()) return;
    if(!operationAttribute) {
        qDebug( ) << Q_FUNC_INFO << "Reply " << operationAttribute << " not for me";
        return;
    }
    if(waitingForAuthentication) {
        qDebug( ) << Q_FUNC_INFO << "Waiting for authentication - ignoring this reply";
        return;
    }
    if(operationAttribute==PEONoOp) {
        Q_ASSERT(false);
    } else if(operationAttribute==PEOLogin) {
        loginReply(reply);
    } else if(operationAttribute==PEOFetchCookie) {
        fetchCookieReply(reply);
    } else if(operationAttribute==PEOUpdateForum) {
        listGroupsReply(reply);
    } else if(operationAttribute==PEOUpdateGroup) {
        listThreadsReply(reply);
    } else if(operationAttribute==PEOUpdateThread) {
        listMessagesReply(reply);
    }
}

void ParserEngine::fetchCookie() {
    Q_ASSERT(parser()->forum_url.length() > 0);
    if (operationInProgress == PEONoOp)
        return;
    QNetworkRequest req(QUrl(parser()->forum_url));
    setRequestAttributes(req, PEOFetchCookie);
    nam->post(req, emptyData);
}


void ParserEngine::fetchCookieReply(QNetworkReply *reply) {
    if(operationInProgress == PEONoOp) return;

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << Q_FUNC_INFO << reply->errorString();
        emit(networkFailure(reply->errorString()));
        cancelOperation();
        return;
    }
    if (operationInProgress == PEONoOp)
        return;
    cookieFetched = true;
    cookieExpiredTimer.start();
    nextOperation();
}

void ParserEngine::loginToForum() {
    qDebug() << Q_FUNC_INFO << subscription()->toString();
    Q_ASSERT(!threadBeingUpdated);
    Q_ASSERT(!groupBeingUpdated);
    Q_ASSERT(!loggedIn);
    Q_ASSERT(!loggingIn);
    if (parser()->login_type == ForumParser::LoginTypeNotSupported) {
        qDebug() << Q_FUNC_INFO << "Login not supproted!";
        emit loginFinished(subscription(), false);
        return;
    }

    if (subscription()->username().length() <= 0 || subscription()->password().length() <= 0) {
        qDebug() << Q_FUNC_INFO << "Warning, no credentials supplied. Logging in should fail.";
    }

    QUrl loginUrl(getLoginUrl());
    if (parser()->login_type == ForumParser::LoginTypeHttpPost) {
        QNetworkRequest req;
        req.setUrl(loginUrl);
        setRequestAttributes(req, PEOLogin);
        QHash<QString, QString> params;
        QStringList loginParamPairs = parser()->login_parameters.split(",", QString::SkipEmptyParts);
        foreach(QString paramPair, loginParamPairs) {
            paramPair = paramPair.replace("%u", subscription()->username());
            paramPair = paramPair.replace("%p", subscription()->password());

            if (paramPair.contains('=')) {
                QStringList singleParam = paramPair.split('=', QString::KeepEmptyParts);
                if (singleParam.size() == 2) {
                    params.insert(singleParam.at(0), singleParam.at(1));
                } else {
                    qDebug() << Q_FUNC_INFO << "hm, invalid login parameter pair!";
                }
            }
        }
        loginData = HttpPost::setPostParameters(&req, params);
        loggingIn = true;
        nam->post(req, loginData);
    } else {
        qDebug() << Q_FUNC_INFO << "Sorry, http auth not yet implemented.";
    }
}

void ParserEngine::loginReply(QNetworkReply *reply) {
    qDebug() << Q_FUNC_INFO << subscription()->toString();
    Q_ASSERT(!loggedIn);
    Q_ASSERT(loggingIn);
    loggingIn = false;

    QString data = convertCharset(reply->readAll());
    if (reply->error() != QNetworkReply::NoError) {
        emit(networkFailure(reply->errorString()));
        emit loginFinished(subscription(), false);
        return;
    }
    performLogin(data);
}

void ParserEngine::performLogin(QString &html) {
    //    qDebug() << Q_FUNC_INFO << " looking for " << parser()->verify_login_pattern;
    emit receivedHtml(html);
    bool success = html.contains(parser()->verify_login_pattern);
    //    if(success)
    //        qDebug() << Q_FUNC_INFO << "found in html at " << html.indexOf(parser()->verify_login_pattern);
    loggedIn = success;
    emit loginFinished(subscription(), success);
    // Rest is handled in UpdateEngine
}

void ParserEngine::listGroupsReply(QNetworkReply *reply) {
    if(subscription())
        qDebug() << Q_FUNC_INFO << subscription()->toString();
    Q_ASSERT(!threadBeingUpdated);
    Q_ASSERT(!groupBeingUpdated);
    if(operationInProgress == PEONoOp) return;
    if(operationInProgress != PEOUpdateForum) {
        qWarning() << Q_FUNC_INFO << "ALERT!! POSSIBLE BUG: operationInProgress is not FSOListGroups! It is: " << operationInProgress << " ignoring this signal, i hope nothing breaks.";
        Q_ASSERT(false);
        return;
    }
    Q_ASSERT(reply->request().attribute(QNetworkRequest::User).toInt() == PEOUpdateForum);

    QString data = convertCharset(reply->readAll());
    if (reply->error() != QNetworkReply::NoError) {
        emit(networkFailure(reply->errorString()));
        cancelOperation();
        return;
    }
    performListGroups(data);
}

void ParserEngine::performListGroups(QString &html) {
    Q_ASSERT(!threadBeingUpdated);
    Q_ASSERT(!groupBeingUpdated);
    // Parser maker may need this
    if(operationInProgress == PEONoOp) operationInProgress = PEOUpdateForum;
    emit receivedHtml(html);
    QList<ForumGroup*> groups;
    patternMatcher->setPattern(parser()->group_list_pattern);
    QList<QHash<QString, QString> > matches = patternMatcher->findMatches(html);
    QHash<QString, QString> match;
    foreach (match, matches) {
        ForumGroup *fg = new ForumGroup(this);
        fg->setId(match["%a"]);
        fg->setName(match["%b"]);
        fg->setLastchange(match["%c"]);
        groups.append(fg);
    }
    operationInProgress = PEONoOp;
    emit(listGroupsFinished(groups, subscription()));
    qDeleteAll(groups);
}


void ParserEngine::listThreadsOnNextPage() {
    Q_ASSERT(operationInProgress==PEOUpdateGroup);
    if (operationInProgress != PEOUpdateGroup)
        return;

    QString urlString = getThreadListUrl(groupBeingUpdated, currentListPage);
    QNetworkRequest req;
    req.setUrl(QUrl(urlString));
    setRequestAttributes(req, PEOUpdateGroup);
    nam->post(req, emptyData);
}

void ParserEngine::listThreadsReply(QNetworkReply *reply) {
    if(operationInProgress == PEONoOp) return;
    Q_ASSERT(groupBeingUpdated);
    Q_ASSERT(!threadBeingUpdated);
    Q_ASSERT(operationInProgress == PEOUpdateGroup);
    Q_ASSERT(reply->request().attribute(QNetworkRequest::User).toInt()==PEOUpdateGroup);
    if (reply->error() != QNetworkReply::NoError) {
        emit(networkFailure(reply->errorString()));
        cancelOperation();
        return;
    }
    QString data = convertCharset(reply->readAll());
    performListThreads(data);
}

void ParserEngine::performListThreads(QString &html) {
    // Parser maker may need this
    if(operationInProgress == PEONoOp) operationInProgress = PEOUpdateGroup;
    QList<ForumThread*> newThreads;
    emit receivedHtml(html);
    patternMatcher->setPattern(parser()->thread_list_pattern);
    QList<QHash<QString, QString> > matches = patternMatcher->findMatches(html);
    // Iterate through matches on page
    QHash<QString, QString> match;
    foreach(match, matches) {
        ForumThread *ft = new ForumThread(this);
        ft->setId(match["%a"]);
        ft->setName(match["%b"]);
        ft->setLastchange(match["%c"]);
        ft->setGetMessagesCount(groupBeingUpdated->subscription()->latestMessages());
        ft->setHasMoreMessages(false);
        if (ft->isSane()) {
            newThreads.append(ft);
        } else {
            qDebug() << Q_FUNC_INFO << "Incomplete thread in " << subscription()->toString() << ", not adding";
            // Q_ASSERT(false);
        }
    }
    // See if the new threads contains already unknown threads
    bool newThreadsFound = false;
    // @todo this could be optimized a lot
    while(!newThreads.isEmpty()) {
        ForumThread *newThread = newThreads.takeFirst();
        bool threadFound = false;
        foreach (ForumThread *thread, foundThreads) {
            if (newThread->id() == thread->id()) {
                threadFound = true;
            }
        }
        if(threadFound) { // Delete if already known thread
            delete newThread;
            newThread = 0;
        } else {
            newThreadsFound = true;
            newThread->setOrdernum(foundThreads.size());
            if (foundThreads.size() < subscription()->latestThreads()) {
                foundThreads.append(newThread);
                newThread = 0;
            } else {
                //                qDebug() << "Number of threads exceeding maximum latest threads limit - not adding "
                //                        << newThread->toString();
                newThreadsFound = false;
                delete newThread;
                newThread = 0;
            }
        }
        Q_ASSERT(!newThread);
    }
    bool finished = false;
    if (newThreadsFound) {
        if (parser()->thread_list_page_increment > 0) {
            // Continue to next page
            currentListPage += parser()->thread_list_page_increment;
            //qDebug() << Q_FUNC_INFO << "New threads were found - continuing to next page " << currentListPage;
            listThreadsOnNextPage();
        } else {
            // qDebug() << Q_FUNC_INFO << "Forum doesn't support multipage - NOT continuing to next page.";
            finished = true;
        }
    } else { // Not continuing to next page
        // qDebug() << Q_FUNC_INFO << "No new threads - finished";
        finished = true;
    }
    if (finished) {
        operationInProgress = PEONoOp;
        ForumGroup *updatedGroup = groupBeingUpdated;
        groupBeingUpdated = 0;
        emit listThreadsFinished(foundThreads, updatedGroup);
        qDeleteAll(foundThreads);
        foundThreads.clear();
    }
}

void ParserEngine::listMessagesOnNextPage() {
    Q_ASSERT(operationInProgress==PEOUpdateThread);
    if (operationInProgress != PEOUpdateThread) {
        Q_ASSERT(false);
        return;
    }
    QString urlString = getMessageListUrl(threadBeingUpdated, currentListPage);
    currentMessagesUrl = urlString;

    QNetworkRequest req;
    req.setUrl(QUrl(urlString));
    setRequestAttributes(req, PEOUpdateThread);

    nam->post(req, emptyData);
}

void ParserEngine::listMessagesReply(QNetworkReply *reply) {
    if(operationInProgress == PEONoOp) return;
    Q_ASSERT(operationInProgress == PEOUpdateThread);
    Q_ASSERT(reply->request().attribute(QNetworkRequest::User).toInt() == PEOUpdateThread);
    if (reply->error() != QNetworkReply::NoError) {
        emit(networkFailure(reply->errorString()));
        cancelOperation();
        return;
    }
    QString data = convertCharset(reply->readAll());
    performListMessages(data);
}

void ParserEngine::performListMessages(QString &html) {
    // Parser maker may need this
    if(operationInProgress == PEONoOp) operationInProgress = PEOUpdateThread;
    Q_ASSERT(operationInProgress == PEOUpdateThread);
    //    qDebug() << Q_FUNC_INFO << html;
    QList<ForumMessage*> newMessages;
    Q_ASSERT(threadBeingUpdated->isSane());
    emit receivedHtml(html);
    operationInProgress = PEOUpdateThread;
    patternMatcher->setPattern(parser()->message_list_pattern);
    QList<QHash<QString, QString> > matches = patternMatcher->findMatches(html);
    QHash<QString, QString> match;

    foreach(match, matches){
        // This will be deleted or added to messages
        ForumMessage *fm = new ForumMessage(this);
        fm->setRead(false, false);
        Q_ASSERT(!fm->isRead());
        fm->setId(match["%a"]);
        fm->setName(match["%b"]);
        fm->setBody(match["%c"]);
        fm->setAuthor(match["%d"]);
        fm->setLastchange(match["%e"]);
        if (parser()->supportsMessageUrl()) {
            fm->setUrl(getMessageUrl(fm));
        } else {
            fm->setUrl(currentMessagesUrl);
        }
        if (fm->isSane()) {
            newMessages.append(fm);
        } else {
            qDebug() << Q_FUNC_INFO << "Incomplete message in " << subscription()->toString() << ", not adding";
            // Q_ASSERT(false);
        }
    }
    // See if the new threads contains already unknown threads
    bool newMessagesFound = false;
    while(!newMessages.isEmpty()) {
        ForumMessage *newMessage = newMessages.takeFirst();
        bool messageFound = false;
        for(int idx=0;idx < foundMessages.size() && !messageFound;idx++) {
            ForumMessage *message = foundMessages.value(idx);
            if (newMessage->id() == message->id()) {
                messageFound = true;
            }
        }
        if (messageFound) {
            // Message already in messages - discard it
            delete newMessage;
            newMessage = 0;
        } else {
            // Message not in messages - add it if possible
            newMessagesFound = true;
            newMessage->setOrdernum(foundMessages.size());
            // Check if message limit has reached
            if (foundMessages.size() < threadBeingUpdated->getMessagesCount()) {
                foundMessages.append(newMessage);
                newMessage = 0;
            } else {
                //  qDebug() << "Number of messages exceeding maximum latest messages limit - not adding.";
                newMessagesFound = false;
                moreMessagesAvailable = true;
                delete newMessage;
                newMessage = 0;
            }
        }
        Q_ASSERT(!newMessage);
    }
    bool finished = false;
    if (newMessagesFound) {
        if (parser()->view_thread_page_increment > 0) {
            // Continue to next page
            threadBeingUpdated->setLastPage(currentListPage); // To be updated to db in listMessagesFinished
            currentListPage += parser()->view_thread_page_increment;
            listMessagesOnNextPage();
        } else {
            // qDebug() << "Forum doesn't support multipage - NOT continuing to next page.";
            finished = true;
        }
    } else {
        // qDebug() << "NOT continuing to next page, no new messages found on this one.";
        finished = true;
    }

    if(finished) {
        operationInProgress = PEONoOp;
        ForumThread *threadUpdated = threadBeingUpdated;
        threadBeingUpdated = 0;
        emit listMessagesFinished(foundMessages, threadUpdated, moreMessagesAvailable);
        qDeleteAll(foundMessages);
        foundMessages.clear();
    } else {
        // Help keep UI responsive
        //QCoreApplication::processEvents();
    }
}

QString ParserEngine::statusReport() {
    QString op;
    if (operationInProgress == PEONoOp)
        op = "NoOp";
    if (operationInProgress == PEOUpdateForum)
        op = "ListGroups";
    if (operationInProgress == PEOUpdateGroup)
        op = "UpdateThreads";
    if (operationInProgress == PEOUpdateThread)
        op = "UpdateMessages";

    return "Operation: " + operationNames[operationInProgress] + " in " + parser()->toString() + "\n" + "Threads: "
            + QString().number(foundThreads.size()) + "\n" + "Messages: "
            + QString().number(foundMessages.size()) + "\n" + "Page: "
            + QString().number(currentListPage)/* + "\n" + "Group: "
                                   + currentGroup->toString() + "\n" + "Thread: "
                                                        + currentThread->toString() + "\n"*/;
}

QString ParserEngine::getMessageUrl(const ForumMessage *msg) {
    QUrl url = QUrl();

    QString urlString = parser()->view_message_path;
    urlString = urlString.replace("%g", groupBeingUpdated->id());
    urlString = urlString.replace("%t", threadBeingUpdated->id());
    urlString = urlString.replace("%m", msg->id());
    urlString = parser()->forumUrlWithoutEnd() + urlString;
    return urlString;
}

QString ParserEngine::getLoginUrl() {
    return parser()->forumUrlWithoutEnd() + parser()->login_path;
}

QString ParserEngine::getThreadListUrl(const ForumGroup *grp, int page) {
    QString urlString = parser()->thread_list_path;
    urlString = urlString.replace("%g", grp->id());
    if (parser()->supportsThreadPages()) {
        if (page < 0)
            page = parser()->thread_list_page_start;
        urlString = urlString.replace("%p", QString().number(page));
    }
    urlString = parser()->forumUrlWithoutEnd() + urlString;
    return urlString;
}

QString ParserEngine::getMessageListUrl(const ForumThread *thread, int page) {
    QString urlString = parser()->view_thread_path;
    urlString = urlString.replace("%g", thread->group()->id());
    urlString = urlString.replace("%t", thread->id());
    if (parser()->supportsMessagePages()) {
        if (page < 0)
            page = parser()->view_thread_page_start;
        urlString = urlString.replace("%p", QString().number(page));
    }
    urlString = parser()->forumUrlWithoutEnd() + urlString;
    return urlString;
}

void ParserEngine::authenticationRequired(QNetworkReply * reply, QAuthenticator * authenticator) {
    Q_UNUSED(reply);
    if(operationInProgress == PEONoOp) return;
    int forumId = reply->request().attribute(FORUMID_ATTRIBUTE).toInt();
    if(forumId != subscription()->forumId()) return;

    qDebug() << Q_FUNC_INFO << reply << authenticator;
    if(waitingForAuthentication) {
        qDebug() << Q_FUNC_INFO << "Already waiting for authentication - ignoring this.";
        return;
    }
    if(parser()->login_type == ForumParser::LoginTypeHttpAuth) {
        if (subscription()->username().length() <= 0 || subscription()->password().length() <= 0) {
            qDebug() << Q_FUNC_INFO << "FAIL: no credentials given for subscription " << subscription()->toString();
            cancelOperation();
            emit networkFailure("Server requested for username and password for forum "
                                + subscription()->alias() + " but you haven't provided them.");
        } else {
            qDebug() << Q_FUNC_INFO << "Gave credentials to server";
            authenticator->setUser(subscription()->username());
            authenticator->setPassword(subscription()->password());
        }
    } else {
        waitingForAuthentication = true;
        emit getHttpAuthentication(subscription(), authenticator);
        if(!authenticator->user().isEmpty()) { // Got authentication directly (from settings)
            waitingForAuthentication = false;
        }
    }
}

bool ParserEngine::prepareForUse() {
    if (!cookieFetched) {
        fetchCookie();
        return true;
    }
    if (!loggedIn && parser()->supportsLogin() && subscription()->username().length() > 0
            && subscription()->password().length() > 0) {
        if(!loggingIn)
            loginToForum();
        return true;
    }
    return false;
}

void ParserEngine::nextOperation() {
    switch (operationInProgress) {
    case PEOUpdateForum:
        Q_ASSERT(!groupBeingUpdated);
        Q_ASSERT(!threadBeingUpdated);
        Q_ASSERT(state() == UES_UPDATING || parserMakerMode());
        doUpdateForum();
        break;
    case PEOUpdateGroup:
        Q_ASSERT(groupBeingUpdated);
        Q_ASSERT(state() == UES_UPDATING || parserMakerMode());
        doUpdateGroup(groupBeingUpdated);
        break;
    case PEOUpdateThread:
        Q_ASSERT(threadBeingUpdated);
        Q_ASSERT(state() == UES_UPDATING || parserMakerMode());
        doUpdateThread(threadBeingUpdated);
        break;
    case PEONoOp:
        break;
    default:
        Q_ASSERT(false);
    }
}

void ParserEngine::cookieExpired() {
    cookieFetched = false;
}

void ParserEngine::setRequestAttributes(QNetworkRequest &req, ParserEngineOperation op) {
    req.setAttribute(QNetworkRequest::User, op);
    req.setAttribute(FORUMID_ATTRIBUTE, subscription()->forumId());
    req.setHeader(QNetworkRequest::ContentTypeHeader, QString("application/x-www-form-urlencoded"));
    // @todo Make this configurable
    req.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux i686; rv:6.0) Gecko/20100101 Firefox/6.0");
}

QString ParserEngine::convertCharset(const QByteArray &src) {
    QString converted;
    // I don't like this.. Support needed for more!
    if (parser()->charset == "" || parser()->charset == "utf-8") {
        converted = QString::fromUtf8(src.data());
    } else if (parser()->charset == "iso-8859-1" || parser()->charset == "iso-8859-15") {
        converted = QString::fromLatin1(src.data());
    } else {
        qDebug() << Q_FUNC_INFO << "Unknown charset " << parser()->charset << " - assuming UTF-8";
        converted = QString().fromUtf8(src.data());
    }
    // Remove silly newlines
    converted.replace(QChar(13), QChar(' '));
    return converted;
}

ForumSubscriptionParsed *ParserEngine::subscriptionParsed() const
{
    return qobject_cast<ForumSubscriptionParsed*>(subscription());
}
