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
#ifndef FORUMSESSION_H_
#define FORUMSESSION_H_
#include <QObject>
#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkProxy>
#include <QNetworkCookieJar>
#include <QDebug>
#include <QAuthenticator>
#include <QTimer>

class ForumParser;
class ForumSubscription;
class PatternMatcher;
class ForumGroup;
class ForumThread;
class ForumMessage;

#define FORUMID_ATTRIBUTE (QNetworkRequest::Attribute(QNetworkRequest::User + 1))

/**
  * ForumSession makes HTTP requests to forum, uses
  * patternmatcher to scrape data from the returned HTML and
  * returns wanted data such as threads and messages. ForumSession is used
  * by ParserEngine.
  *
  * @see ParserEngine
  */
class ForumSession : public QObject {
    Q_OBJECT
public:
    enum ForumSessionOperation { FSONoOp=1, FSOLogin, FSOFetchCookie, FSOListGroups, FSOListThreads, FSOListMessages };
    ForumSession(QObject *parent, QNetworkAccessManager *n);
    virtual ~ForumSession();
    void initialize(ForumParser *fop, ForumSubscription *fos, PatternMatcher *matcher=0);
    void setParser(ForumParser *fop);
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
    void performLogin(QString &html);

public slots:
    void networkReply(QNetworkReply *reply);
    void cancelOperation();
    void authenticationRequired (QNetworkReply * reply, QAuthenticator * authenticator); // Called by NAM
    void authenticationReceived();
signals:
    void listGroupsFinished(QList<ForumGroup*> &groups, ForumSubscription *sub);
    void listThreadsFinished(QList<ForumThread*> &threads, ForumGroup *group); // Will be deleted
    void listMessagesFinished(QList<ForumMessage*> &messages, ForumThread *thread, bool moreAvailable);
    void networkFailure(QString message);
    void loginFinished(ForumSubscription *sub, bool success);
    void receivedHtml(const QString &data);
    // Asynchronous
    void getHttpAuthentication(ForumSubscription *fsub, QAuthenticator *authenticator);
private slots:
    void cookieExpired(); // Called when cookie needs to be fetched again
private:
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
    void setRequestAttributes(QNetworkRequest &req, ForumSessionOperation op);

    QString convertCharset(const QByteArray &src);
    QString statusReport();
    PatternMatcher *pm;
    ForumParser *fpar;
    ForumSubscription *fsub;
    QNetworkAccessManager *nam;
    QByteArray emptyData, loginData;
    bool cookieFetched, loggedIn;
    ForumSessionOperation operationInProgress;
    QNetworkCookieJar *cookieJar;
    int currentListPage;
    ForumGroup *currentGroup;
    ForumThread *currentThread;

    // @todo consider changing to QVectors
    QList<ForumThread*> threads; // Threads in currentGroup
    QList<ForumMessage*> messages; // Represents messages in thread listMessages

    bool moreMessagesAvailable; // True if thread would have more messages but limit stops search
    QString currentMessagesUrl;
    QTimer cookieExpiredTimer;
    bool waitingForAuthentication;
};

#endif /* FORUMSESSION_H_ */
