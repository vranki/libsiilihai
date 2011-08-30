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
#include "forumsession.h"

ForumSession::ForumSession(QObject *parent, QNetworkAccessManager *n) : QObject(parent), nam(n) {
    operationInProgress = FSONoOp;
    cookieJar = 0;
    currentListPage = 0;
    pm = 0;
    fsub = 0;
    loggedIn = false;
    cookieFetched = false;
    clearAuthentications();
    cookieExpiredTimer.setSingleShot(true);
    cookieExpiredTimer.setInterval(10*60*1000);
    connect(&cookieExpiredTimer, SIGNAL(timeout()), this, SLOT(cookieExpired()));
}

void ForumSession::initialize(ForumParser *fop, ForumSubscription *fos, PatternMatcher *matcher) {
    Q_ASSERT(fos);
    fsub = fos;
    fpar = fop;

    cookieFetched = false;
    operationInProgress = FSONoOp;
    pm = matcher;
    if (!pm)
        pm = new PatternMatcher(this);
    nam->setProxy(QNetworkProxy::applicationProxy());
    connect(nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkReply(QNetworkReply*)), Qt::UniqueConnection);
}

ForumSession::~ForumSession() {
}

void ForumSession::networkReply(QNetworkReply *reply) {
    int operationAttribute = reply->request().attribute(QNetworkRequest::User).toInt();
    if(!operationAttribute) {
        qDebug( ) << Q_FUNC_INFO << "Reply " << operationAttribute << " not for me";
        return;
    }
    if(operationAttribute==FSONoOp) {
        Q_ASSERT(false);
    } else if(operationAttribute==FSOLogin) {
        loginReply(reply);
    } else if(operationAttribute==FSOFetchCookie) {
        fetchCookieReply(reply);
    } else if(operationAttribute==FSOListGroups) {
        listGroupsReply(reply);
    } else if(operationAttribute==FSOListThreads) {
        listThreadsReply(reply);
    } else if(operationAttribute==FSOListMessages) {
        listMessagesReply(reply);
    }
}

QString ForumSession::convertCharset(const QByteArray &src) {
    QString converted;
    // I don't like this.. Support needed for more!
    if (fpar->charset == "" || fpar->charset == "utf-8") {
        converted = QString::fromUtf8(src.data());
    } else if (fpar->charset == "iso-8859-1" || fpar->charset == "iso-8859-15") {
        converted = QString::fromLatin1(src.data());
    } else {
        qDebug() << "Unknown charset " << fpar->charset << " - assuming ASCII";
        converted = QString().fromAscii(src.data());
    }
    // Remove silly newlines
    converted.replace(QChar(13), QChar(' '));
    return converted;
}

void ForumSession::listGroups() {
    qDebug() << Q_FUNC_INFO << fpar->forum_url;
    if (operationInProgress != FSONoOp && operationInProgress != FSOListGroups) {
        qDebug() << Q_FUNC_INFO << "Operation in progress!! Don't command me yet! ";
        return;
    }
    operationInProgress = FSOListGroups;
    if(prepareForUse()) return;

    QNetworkRequest req(QUrl(fpar->forum_url));
    req.setAttribute(QNetworkRequest::User, FSOListGroups);
    nam->post(req, emptyData);
}

void ForumSession::listGroupsReply(QNetworkReply *reply) {
    if(operationInProgress == FSONoOp) return;
    Q_ASSERT(operationInProgress==FSOListGroups);
    Q_ASSERT(reply->request().attribute(QNetworkRequest::User).toInt() ==FSOListGroups);

    qDebug() << Q_FUNC_INFO;

    QString data = convertCharset(reply->readAll());
    if (reply->error() != QNetworkReply::NoError) {
        emit(networkFailure(reply->errorString()));
        cancelOperation();
        return;
    }
    performListGroups(data);
}

void ForumSession::performListGroups(QString &html) {
    Q_ASSERT(pm);
    emit receivedHtml(html);
    QList<ForumGroup*> groups;
    pm->setPattern(fpar->group_list_pattern);
    QList<QHash<QString, QString> > matches = pm->findMatches(html);
    QHash<QString, QString> match;
    foreach (match, matches) {
        ForumGroup *fg = new ForumGroup(fsub);
        fg->setId(match["%a"]);
        fg->setName(match["%b"]);
        fg->setLastchange(match["%c"]);
        groups.append(fg);
    }
    operationInProgress = FSONoOp;
    emit(listGroupsFinished(groups));
    qDeleteAll(groups);
}


void ForumSession::fetchCookieReply(QNetworkReply *reply) {
    if(operationInProgress == FSONoOp) return;

    qDebug() << Q_FUNC_INFO;
    //  qDebug() << statusReport();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << Q_FUNC_INFO << reply->errorString();
        emit(networkFailure(reply->errorString()));
        cancelOperation();
        return;
    }
    if (operationInProgress == FSONoOp)
        return;
    /*
 QList<QNetworkCookie> cookies = cookieJar->cookiesForUrl(QUrl(
   fpar->forum_url));
 for (int i = 0; i < cookies.size(); i++) {
  qDebug() << "\t" << cookies[i].name();
 }
 */
    cookieFetched = true;
    cookieExpiredTimer.start();
    nextOperation();
}

void ForumSession::loginToForum() {
    qDebug() << Q_FUNC_INFO;

    if (fpar->login_type == ForumParser::LoginTypeNotSupported) {
        qDebug() << "Login not supproted!";
        emit loginFinished(fsub, false);
        return;
    }

    if (fsub->username().length() <= 0 || fsub->password().length() <= 0) {
        qDebug() << "Warning, no credentials supplied. Logging in should fail.";
    }

    QUrl loginUrl(getLoginUrl());
    if (fpar->login_type == ForumParser::LoginTypeHttpPost) {
        QNetworkRequest req;
        req.setUrl(loginUrl);
        req.setAttribute(QNetworkRequest::User, FSOLogin);
        QHash<QString, QString> params;
        QStringList loginParamPairs = fpar->login_parameters.split(",", QString::SkipEmptyParts);
        foreach(QString paramPair, loginParamPairs) {
            paramPair = paramPair.replace("%u", fsub->username());
            paramPair = paramPair.replace("%p", fsub->password());

            if (paramPair.contains('=')) {
                QStringList singleParam = paramPair.split('=', QString::KeepEmptyParts);
                if (singleParam.size() == 2) {
                    params.insert(singleParam.at(0), singleParam.at(1));
                } else {
                    qDebug("hm, invalid login parameter pair!");
                }
            }
        }
        loginData = HttpPost::setPostParameters(&req, params);

        nam->post(req, loginData);
    } else {
        qDebug("Sorry, http auth not yet implemented.");
    }
}

void ForumSession::loginReply(QNetworkReply *reply) {
    qDebug() << Q_FUNC_INFO;
    // qDebug() << statusReport();

    QString data = convertCharset(reply->readAll());

    if (reply->error() != QNetworkReply::NoError) {
        emit(networkFailure(reply->errorString()));
        loginFinished(fsub, false);
        cancelOperation();
        return;
    }

    performLogin(data);
}

void ForumSession::performLogin(QString &html) {
    qDebug() << Q_FUNC_INFO << " looking for " << fpar->verify_login_pattern;
    emit receivedHtml(html);
    bool success = html.contains(fpar->verify_login_pattern);
    if(success)
        qDebug() << "found in html at " << html.indexOf(fpar->verify_login_pattern);
    emit loginFinished(fsub, success);
    loggedIn = success;
    if(loggedIn) {
        nextOperation();
    } else {
        cancelOperation();
        qDebug() << "Login failed - cancelling ops!";
    }
}

void ForumSession::fetchCookie() {
    qDebug() << Q_FUNC_INFO << fpar->forum_url;
    Q_ASSERT(fpar->forum_url.length() > 0);
    if (operationInProgress == FSONoOp)
        return;
    QNetworkRequest req(QUrl(fpar->forum_url));
    req.setAttribute(QNetworkRequest::User, QVariant(FSOFetchCookie));
    nam->post(req, emptyData);
}

void ForumSession::listThreadsOnNextPage() {
    Q_ASSERT(operationInProgress==FSOListThreads);
    if (operationInProgress != FSOListThreads)
        return;

    QString urlString = getThreadListUrl(currentGroup, currentListPage);
    QNetworkRequest req;
    req.setUrl(QUrl(urlString));
    req.setAttribute(QNetworkRequest::User, FSOListThreads);
    qDebug() << Q_FUNC_INFO << "Fetching URL" << urlString;
    nam->post(req, emptyData);
}

void ForumSession::listMessagesOnNextPage() {
    Q_ASSERT(operationInProgress==FSOListMessages);
    if (operationInProgress != FSOListMessages) {
        Q_ASSERT(false);
        return;
    }
    QString urlString = getMessageListUrl(currentThread, currentListPage);
    currentMessagesUrl = urlString;

    QNetworkRequest req;
    req.setUrl(QUrl(urlString));
    req.setAttribute(QNetworkRequest::User, FSOListMessages);
    qDebug() << Q_FUNC_INFO << "Fetching URL" << urlString;

    nam->post(req, emptyData);
}

void ForumSession::listThreads(ForumGroup *group) {
    qDebug() << Q_FUNC_INFO << group->toString();

    if (operationInProgress != FSONoOp && operationInProgress != FSOListThreads) {
        //statusReport();
        qDebug() << Q_FUNC_INFO << "Operation in progress!! Don't command me yet!";
        Q_ASSERT(false);
        return;
    }
    operationInProgress = FSOListThreads;
    currentGroup = group;
    currentMessagesUrl = QString::null;
    if(prepareForUse()) return;
    currentListPage = fpar->thread_list_page_start;
    listThreadsOnNextPage();
}

void ForumSession::listThreadsReply(QNetworkReply *reply) {
    if(operationInProgress == FSONoOp) return;
    Q_ASSERT(currentGroup);
    Q_ASSERT(operationInProgress == FSOListThreads);
    Q_ASSERT(reply->request().attribute(QNetworkRequest::User).toInt() ==FSOListThreads);
    qDebug() << Q_FUNC_INFO << currentGroup->toString();
    if (reply->error() != QNetworkReply::NoError) {
        emit(networkFailure(reply->errorString()));
        cancelOperation();
        return;
    }
    QString data = convertCharset(reply->readAll());
    performListThreads(data);
}
void ForumSession::listMessages(ForumThread *thread) {
    Q_ASSERT(thread->isSane());
    if (operationInProgress != FSONoOp && operationInProgress != FSOListMessages) {
        qDebug() << Q_FUNC_INFO << "Operation in progress!! Don't command me yet!";
        Q_ASSERT(false);
        return;
    }
    operationInProgress = FSOListMessages;
    currentThread = thread;
    currentGroup = thread->group();

    messages.clear();
    moreMessagesAvailable = false;
    if(prepareForUse()) return;
    /* @todo enable this when it's figured out how to add the new messages to
      end of thread and NOT delete the messages in beginning of thread.

    if(thread->getLastPage()) { // Start from last known page if possible
        currentListPage = thread->getLastPage();
    } else {*/
    currentListPage = fpar->view_thread_page_start;
  //  }

    listMessagesOnNextPage();
}

void ForumSession::listMessagesReply(QNetworkReply *reply) {
    if(operationInProgress == FSONoOp) return;
    qDebug() << Q_FUNC_INFO << currentMessagesUrl;
    Q_ASSERT(operationInProgress == FSOListMessages);
    Q_ASSERT(reply->request().attribute(QNetworkRequest::User).toInt() ==FSOListMessages);
    if (reply->error() != QNetworkReply::NoError) {
        emit(networkFailure(reply->errorString()));
        cancelOperation();
        return;
    }
    QString data = convertCharset(reply->readAll());
    performListMessages(data);
}

void ForumSession::performListMessages(QString &html) {
    QList<ForumMessage*> newMessages;
    Q_ASSERT(currentThread->isSane());
    emit receivedHtml(html);
    operationInProgress = FSOListMessages;
    pm->setPattern(fpar->message_list_pattern);
    QList<QHash<QString, QString> > matches = pm->findMatches(html);
    QHash<QString, QString> match;
    foreach(match, matches){
        // This will be deleted or added to messages
        ForumMessage *fm = new ForumMessage(currentThread);
        fm->setRead(false, false);
        fm->setId(match["%a"]);
        fm->setName(match["%b"]);
        fm->setBody(match["%c"]);
        fm->setAuthor(match["%d"]);
        fm->setLastchange(match["%e"]);
        if (fpar->supportsMessageUrl()) {
            fm->setUrl(getMessageUrl(fm));
        } else {
            fm->setUrl(currentMessagesUrl);
        }
        if (fm->isSane()) {
            newMessages.append(fm);
        } else {
            qDebug() << "Incomplete message, not adding";
            // Q_ASSERT(false);
        }
    }
    // See if the new threads contains already unknown threads
    bool newMessagesFound = false;
    while(!newMessages.isEmpty()) {
        ForumMessage *newMessage = newMessages.takeFirst();
        bool messageFound = false;
        foreach (ForumMessage *message, messages) {
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
            newMessage->setOrdernum(messages.size());
            // Check if message limit has reached
            if (messages.size() < currentThread->getMessagesCount()) {
                messages.append(newMessage);
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
        if (fpar->view_thread_page_increment > 0) {
            // Continue to next page
            currentThread->setLastPage(currentListPage); // To be updated to db in listMessagesFinished
            currentListPage += fpar->view_thread_page_increment;
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
        operationInProgress = FSONoOp;
        emit listMessagesFinished(messages, currentThread, moreMessagesAvailable);
        qDeleteAll(messages);
        messages.clear();
    } else {
        // Help keep UI responsive
        QCoreApplication::processEvents();
    }
}

QString ForumSession::statusReport() {
    QString op;
    if (operationInProgress == FSONoOp)
        op = "NoOp";
    if (operationInProgress == FSOListGroups)
        op = "ListGroups";
    if (operationInProgress == FSOListThreads)
        op = "UpdateThreads";
    if (operationInProgress == FSOListMessages)
        op = "UpdateMessages";

    return "Operation: " + op + " in " + fpar->toString() + "\n" + "Threads: "
            + QString().number(threads.size()) + "\n" + "Messages: "
            + QString().number(messages.size()) + "\n" + "Page: "
            + QString().number(currentListPage)/* + "\n" + "Group: "
                   + currentGroup->toString() + "\n" + "Thread: "
                                        + currentThread->toString() + "\n"*/;
}

void ForumSession::performListThreads(QString &html) {
    QList<ForumThread*> newThreads;
    emit receivedHtml(html);
    pm->setPattern(fpar->thread_list_pattern);
    QList<QHash<QString, QString> > matches = pm->findMatches(html);
    qDebug() << Q_FUNC_INFO << "found " << matches.size() << " matches";
    // Iterate through matches on page
    QHash<QString, QString> match;
    foreach(match, matches) {
        ForumThread *ft = new ForumThread(currentGroup);
        ft->setId(match["%a"]);
        ft->setName(match["%b"]);
        ft->setLastchange(match["%c"]);
        ft->setGetMessagesCount(currentGroup->subscription()->latestMessages());
        ft->setHasMoreMessages(false);
        if (ft->isSane()) {
            newThreads.append(ft);
        } else {
            qDebug() << "Incomplete thread, not adding";
            // Q_ASSERT(false);
        }
    }
    // See if the new threads contains already unknown threads
    bool newThreadsFound = false;
    // @todo this could be optimized a lot
    while(!newThreads.isEmpty()) {
        ForumThread *newThread = newThreads.takeFirst();
        bool threadFound = false;
        foreach (ForumThread *thread, threads) {
            if (newThread->id() == thread->id()) {
                threadFound = true;
            }
        }
        if(threadFound) { // Delete if already known thread
            delete newThread;
            newThread = 0;
        } else {
            newThreadsFound = true;
            newThread->setOrdernum(threads.size());
            if (threads.size() < fsub->latestThreads()) {
                threads.append(newThread);
                newThread = 0;
            } else {
                //qDebug() << "Number of threads exceeding maximum latest threads limit - not adding "
                //        << newThread->toString();
                newThreadsFound = false;
                delete newThread;
                newThread = 0;
            }
        }
        Q_ASSERT(!newThread);
    }
    bool finished = false;
    if (newThreadsFound) {
        if (fpar->thread_list_page_increment > 0) {
            // Continue to next page
            currentListPage += fpar->thread_list_page_increment;
            qDebug() << "New threads were found - continuing to next page "
                     << currentListPage;
            listThreadsOnNextPage();
        } else {
            qDebug() << "Forum doesn't support multipage - NOT continuing to next page.";
            finished = true;
        }
    } else {
        qDebug() << "NOT continuing to next page.";
        finished = true;
    }
    if (finished) {
        operationInProgress = FSONoOp;
        emit listThreadsFinished(threads, currentGroup);
        qDeleteAll(threads);
        threads.clear();
    }
}

void ForumSession::cancelOperation() {
    qDebug() << Q_FUNC_INFO;

    operationInProgress = FSONoOp;
    cookieFetched = false;
    currentListPage = -1;
    qDeleteAll(messages);
    messages.clear();
    qDeleteAll(threads);
    threads.clear();
    currentGroup = 0;
    currentThread = 0;
}

QString ForumSession::getMessageUrl(const ForumMessage *msg) {
    QUrl url = QUrl();

    QString urlString = fpar->view_message_path;
    urlString = urlString.replace("%g", currentGroup->id());
    urlString = urlString.replace("%t", currentThread->id());
    urlString = urlString.replace("%m", msg->id());
    urlString = fpar->forumUrlWithoutEnd() + urlString;
    return urlString;
}

QString ForumSession::getLoginUrl() {
    return fpar->forumUrlWithoutEnd() + fpar->login_path;
}

void ForumSession::setParser(ForumParser *fop) {
    // Sanity check can't be done here as it would break parser maker
    fpar = fop;
}

QString ForumSession::getThreadListUrl(const ForumGroup *grp, int page) {
    QString urlString = fpar->thread_list_path;
    urlString = urlString.replace("%g", grp->id());
    if (fpar->supportsThreadPages()) {
        if (page < 0)
            page = fpar->thread_list_page_start;
        urlString = urlString.replace("%p", QString().number(page));
    }
    urlString = fpar->forumUrlWithoutEnd() + urlString;
    return urlString;
}

QString ForumSession::getMessageListUrl(const ForumThread *thread, int page) {
    QString urlString = fpar->view_thread_path;
    urlString = urlString.replace("%g", thread->group()->id());
    urlString = urlString.replace("%t", thread->id());
    if (fpar->supportsMessagePages()) {
        if (page < 0)
            page = fpar->view_thread_page_start;
        urlString = urlString.replace("%p", QString().number(page));
    }
    urlString = fpar->forumUrlWithoutEnd() + urlString;
    return urlString;
}

void ForumSession::authenticationRequired(QNetworkReply * reply, QAuthenticator * authenticator) {
    Q_UNUSED(reply);
    if(operationInProgress == FSONoOp) return;

    qDebug() << Q_FUNC_INFO;

    if(fpar->login_type == ForumParser::LoginTypeHttpAuth) {
        if (fsub->username().length() <= 0 || fsub->password().length() <= 0) {
            qDebug() << Q_FUNC_INFO << "FAIL: no credentials given for subscription "
                     << fsub->toString();
            cancelOperation();
            emit networkFailure("Server requested for username and password for forum "
                                + fsub->alias() + " but you haven't provided them.");
        } else {
            qDebug() << Q_FUNC_INFO << "Gave credentials to server";
            authenticator->setUser(fsub->username());
            authenticator->setPassword(fsub->password());
        }
    } else {
        emit getAuthentication(fsub, authenticator);
    }
}

void ForumSession::clearAuthentications() {
    cancelOperation();

    if (cookieJar)
        cookieJar->deleteLater();
    loggedIn = false;

    cookieJar = new QNetworkCookieJar();
    nam->setCookieJar(cookieJar);
    connect(nam, SIGNAL(authenticationRequired(QNetworkReply *, QAuthenticator *)),
            this, SLOT(authenticationRequired(QNetworkReply *, QAuthenticator *)), Qt::UniqueConnection);
}

bool ForumSession::prepareForUse() {
    if (!cookieFetched) {
        fetchCookie();
        return true;
    }
    if (!loggedIn && fpar->supportsLogin() && fsub->username().length() > 0 && fsub->password().length() > 0) {
        loginToForum();
        return true;
    }
    return false;
}

void ForumSession::nextOperation() {
    switch (operationInProgress) {
    case FSOListGroups:
        listGroups();
        break;
    case FSOListThreads:
        listThreads(currentGroup);
        break;
    case FSOListMessages:
        listMessages(currentThread);
        break;
    case FSONoOp:
        break;
    default:
        Q_ASSERT(false);
    }
}

void ForumSession::cookieExpired() {
    cookieFetched = false;
}
