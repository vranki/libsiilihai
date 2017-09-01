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
#include "siilihaiprotocol.h"
#include "xmlserialization.h"
#include "parser/forumparser.h"
#include "forumrequest.h"
#include "forumdata/forumsubscription.h"
#include "parser/forumsubscriptionparsed.h"
#include "forumdata/forumgroup.h"
#include "forumdata/forumthread.h"
#include "forumdata/forummessage.h"
#include "httppost.h"
#include "parser/parserreport.h"
#include "parser/forumsubscriptionparsed.h"
#include "usersettings.h"
#include <QUrlQuery>

SiilihaiProtocol::SiilihaiProtocol(QObject *parent) : QObject(parent)
  , operationInProgress(SPONoOp)
  , forumBeingSubscribed(nullptr)
  , m_offline(true)
{
    nam.setCookieJar(new QNetworkCookieJar(this));
    setBaseURL(BASEURL);
    connect(&nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkReply(QNetworkReply*)), Qt::UniqueConnection);
}

SiilihaiProtocol::~SiilihaiProtocol() { }

void SiilihaiProtocol::networkReply(QNetworkReply *reply) {
    int operationAttribute = reply->request().attribute(QNetworkRequest::User).toInt();
    if(!operationAttribute) {
        qDebug( ) << Q_FUNC_INFO << "Reply " << operationAttribute << " not for me";
        return;
    }
    if(operationAttribute==SPONoOp) {
        Q_ASSERT(false);
    } else if(operationAttribute==SPOLogin) {
        replyLogin(reply);
    } else if(operationAttribute==SPORegisterUser) {
        replyLogin(reply);
    } else if(operationAttribute==SPOListForums) {
        replyListForums(reply);
    } else if(operationAttribute==SPOListRequests) {
        replyListRequests(reply);
    } else if(operationAttribute==SPOGetParser) {
        replyGetParser(reply);
    } else if(operationAttribute==SPOSubscribeForum) {
        replySubscribeForum(reply);
    } else if(operationAttribute==SPOSubscribeGroups) {
        replySubscribeGroups(reply);
    } else if(operationAttribute==SPOSaveParser) {
        replySaveParser(reply);
    } else if(operationAttribute==SPOGetUserSettings) {
        replyGetUserSettings(reply);
    } else if(operationAttribute==SPOSetUserSettings) {
        replyGetUserSettings(reply);
    } else if(operationAttribute==SPOSendParserReport) {
        replySendParserReport(reply);
    } else if(operationAttribute==SPOSendThreadData) {
        replySendThreadData(reply);
    } else if(operationAttribute==SPOGetThreadData) {
        replyDownsync(reply);
    } else if(operationAttribute==SPOGetSyncSummary) {
        replyGetSyncSummary(reply);
    } else if(operationAttribute==SPOAddForum || operationAttribute==SPOGetForum ) {
        replyGetForum(reply);
    } else {
        Q_ASSERT(false);
    }
}

QString SiilihaiProtocol::baseURL() {
    return baseUrl;
}

void SiilihaiProtocol::setBaseURL(QString bu) {
    baseUrl = bu;
    listForumsUrl = QUrl(baseUrl + "api/forumlist.xml");
    loginUrl = QUrl(baseUrl + "api/login.xml");
    registerUrl = QUrl(baseUrl + "api/register.xml");
    getParserUrl = QUrl(baseUrl + "api/getparser.xml");
    subscribeForumUrl = QUrl(baseUrl + "api/subscribeforum.xml");
    saveParserUrl = QUrl(baseUrl + "api/saveparser.xml");
    listRequestsUrl = QUrl(baseUrl + "api/requestlist.xml");
    sendParserReportUrl = QUrl(baseUrl + "api/sendparserreport.xml");
    subscribeGroupsUrl = QUrl(baseUrl + "api/subscribegroups.xml");
    sendThreadDataUrl = QUrl(baseUrl + "api/threaddata.xml");
    syncSummaryUrl = QUrl(baseUrl + "api/syncsummary.xml");
    downsyncUrl = QUrl(baseUrl + "api/downsync.xml");
    userSettingsUrl = QUrl(baseUrl + "api/usersettings.xml");
    addForumUrl = QUrl(baseUrl + "api/addforum.xml");
    getForumUrl = QUrl(baseUrl + "api/getforum.xml");
    nam.setProxy(QNetworkProxy::applicationProxy());
}

void SiilihaiProtocol::login(QString user, QString pass) {
    QNetworkRequest req(loginUrl);
    HttpPost post(req, SPOLogin);

    post.addQueryItem("username", user);
    post.addQueryItem("password", pass);
    post.addQueryItem("action", "login");
    post.addQueryItem("clientversion", CLIENT_VERSION);

    nam.post(req, post.postData());
}

void SiilihaiProtocol::replyLogin(QNetworkReply *reply) {
    QString docs = QString().fromUtf8(reply->readAll());
    QString ck = QString::null;
    QString motd = QString::null;
    bool syncEnabled = false;
    if (reply->error() == QNetworkReply::NoError) {
        QDomDocument doc;
        doc.setContent(docs);
        QDomElement re = doc.firstChildElement("login");
        ck = re.firstChildElement("client_key").text();
        motd = re.firstChildElement("motd").text();
        syncEnabled = re.firstChildElement("sync_enabled").text() == "true";
    } else {
        motd = reply->errorString();
    }
    if (ck.length() > 0) clientKey = ck;
    emit loginFinished(isLoggedIn(), motd, syncEnabled);
    reply->deleteLater();
}

// Reply is handled in login
void SiilihaiProtocol::registerUser(QString user, QString pass, QString email, bool sync) {
    QNetworkRequest req(registerUrl);
    HttpPost post(req, SPORegisterUser, QString::null);
    post.addQueryItem("username", user);
    post.addQueryItem("password", pass);
    post.addQueryItem("email", email);
    post.addQueryItem("captcha", "earth"); // @todo something smarter
    post.addQueryItem("clientversion", CLIENT_VERSION);
    if(sync) post.addQueryItem("sync_enabled", "true");
    nam.post(req, post.postData());
}

void SiilihaiProtocol::listForums() {
    QNetworkRequest req(listForumsUrl);
    HttpPost post(req, SPOListForums, clientKey);
    nam.post(req, post.postData());
}

void SiilihaiProtocol::replyListForums(QNetworkReply *reply) {
    QString docs = QString().fromUtf8(reply->readAll());
    qDebug() << docs;
    QList<ForumSubscription*> forums;
    if (reply->error() == QNetworkReply::NoError) {
        QDomDocument doc;
        doc.setContent(docs);
        QDomElement n = doc.firstChildElement("forumlist").firstChildElement("forum");
        while (!n.isNull()) {
            ForumSubscription::ForumProvider provider = static_cast<ForumSubscription::ForumProvider>(n.firstChildElement("provider").text().toInt());
            if(provider > 0 && provider < ForumSubscription::FP_MAX) {
                ForumSubscription *sub = new ForumSubscription(nullptr, true, provider);
                sub->setId(n.firstChildElement("id").text().toInt());
                sub->setAlias(n.firstChildElement("name").text());
                sub->setForumUrl(n.firstChildElement("forum_url").text());
                sub->setSupportsLogin(n.firstChildElement("supports_login").text()=="True");
                forums.append(sub);
            }
            n = n.nextSiblingElement("forum");
        }
    } else {
        qDebug() << Q_FUNC_INFO << "Network error: " << reply->errorString();
        emit networkError(reply->errorString());
    }
    emit(listForumsFinished(forums));
    reply->deleteLater();
}

void SiilihaiProtocol::listRequests() {
    QNetworkRequest req(listRequestsUrl);
    HttpPost post(req, SPOListRequests, clientKey);
    nam.post(req, post.postData());
}

void SiilihaiProtocol::replyListRequests(QNetworkReply *reply) {
    QString docs = QString().fromUtf8(reply->readAll());
    QList<ForumRequest*> requests;
    if (reply->error() == QNetworkReply::NoError) {
        QDomDocument doc;
        doc.setContent(docs);
        QDomElement n = doc.firstChildElement("requestlist").firstChildElement("request");
        while (!n.isNull()) {
            ForumRequest *request = XmlSerialization::readForumRequest(n, this);
            requests.append(request);
            n = n.nextSiblingElement("request");
        }
    } else {
        qDebug() << Q_FUNC_INFO << "Network error: " << reply->errorString();
    }
    emit listRequestsFinished(requests);
    reply->deleteLater();
}
void SiilihaiProtocol::getParser(const int id) {
    Q_ASSERT(id > 0);
    QNetworkRequest req(getParserUrl);
    HttpPost post(req, SPOGetParser, clientKey);
    post.addQueryItem("id", QString().number(id));
    nam.post(req, post.postData());
}

void SiilihaiProtocol::replyGetParser(QNetworkReply *reply) {
    QString docs = QString().fromUtf8(reply->readAll());
    ForumParser *parser = nullptr;
    if (reply->error() == QNetworkReply::NoError) {
        QDomDocument doc;
        doc.setContent(docs);
        QDomElement re = doc.firstChildElement("parser");
        parser = XmlSerialization::readParser(re, this);
        if(parser) {
            parser->update_date = QDate::currentDate();
        } else {
            qDebug() << Q_FUNC_INFO << docs;
            emit networkError(QString("Received malformed parser.\n This should not happen"));
        }
    } else {
        emit networkError(QString("Network error (this should not happen):\n%1").arg(reply->errorString()));
    }
    emit getParserFinished(parser);
    reply->deleteLater();
    if(parser) parser->deleteLater();
}

void SiilihaiProtocol::subscribeForum(ForumSubscription *fs, bool unsubscribe) {
    qDebug() << Q_FUNC_INFO << "unsub: " << unsubscribe << " authenticated: " << fs->isAuthenticated();
    QNetworkRequest req(subscribeForumUrl);
    HttpPost post(req, SPOSubscribeForum, clientKey);
    post.addQueryItem("forum_id", QString().number(fs->id()));
    if (unsubscribe) {
        post.addQueryItem("unsubscribe", "yes");
    } else {
        post.addQueryItem("url", fs->forumUrl().toString());
        post.addQueryItem("provider", QString().number(fs->provider()));
        post.addQueryItem("alias", fs->alias());
        post.addQueryItem("latest_threads", QString().number(fs->latestThreads()));
        post.addQueryItem("latest_messages", QString().number(fs->latestMessages()));
        if(fs->isAuthenticated()) {
            post.addQueryItem("authenticated", "yes");
        }
    }
    forumBeingSubscribed = fs;
    nam.post(req, post.postData());
}

void SiilihaiProtocol::replySubscribeForum(QNetworkReply *reply) {
    Q_ASSERT(forumBeingSubscribed);
    bool success = reply->error() == QNetworkReply::NoError;

    if(!success) {
        qDebug() << Q_FUNC_INFO << "Network error:" << reply->errorString();
    }
    emit subscribeForumFinished(forumBeingSubscribed, success);
    reply->deleteLater();
    forumBeingSubscribed = 0;
}

void SiilihaiProtocol::subscribeGroups(ForumSubscription *fs) {
    QNetworkRequest req(subscribeGroupsUrl);
    QDomDocument doc("SiilihaiML");
    QDomElement root = doc.createElement("SubscribeGroups");
    doc.appendChild(root);

    QDomElement forumTag = doc.createElement("forum");
    root.appendChild(forumTag);

    QDomText t = doc.createTextNode(QString().number(fs->id()));
    forumTag.appendChild(t);

    for(ForumGroup *g : fs->values()) {
        QDomElement subTag;
        if (g->isSubscribed()) {
            subTag = doc.createElement("subscribe");
            subTag.setAttribute("changeset", g->changeset());
        } else {
            subTag = doc.createElement("unsubscribe");
        }
        root.appendChild(subTag);
        QDomText t = doc.createTextNode(g->id());
        subTag.appendChild(t);
    }
    req.setAttribute(QNetworkRequest::User, SPOSubscribeGroups);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QString("application/x-www-form-urlencoded"));
    nam.post(req, doc.toByteArray());
}

void SiilihaiProtocol::replySubscribeGroups(QNetworkReply *reply) {
    bool success = reply->error() == QNetworkReply::NoError;
    emit (subscribeGroupsFinished(success));
    reply->deleteLater();
}

void SiilihaiProtocol::saveParser(const ForumParser *parser) {
    if (!parser->mayWork()) {
        qDebug() << Q_FUNC_INFO << "Tried to save not working parser!!";
        emit saveParserFinished(-666, "That won't work!");
        return;
    }

    // @todo save as XML
    QNetworkRequest req(saveParserUrl);
    HttpPost post(req, SPOSaveParser, clientKey);
    post.addQueryItem("id", QString().number(parser->id()));
    post.addQueryItem("parser_name", parser->name());
    post.addQueryItem("forum_url", parser->forum_url);
    post.addQueryItem("parser_status", QString().number(parser->parser_status));
    post.addQueryItem("thread_list_path", parser->thread_list_path);
    post.addQueryItem("view_thread_path", parser->view_thread_path);
    post.addQueryItem("login_path", parser->login_path);
    post.addQueryItem("date_format", QString().number(parser->date_format));
    post.addQueryItem("group_list_pattern", parser->group_list_pattern);
    post.addQueryItem("thread_list_pattern", parser->thread_list_pattern);
    post.addQueryItem("message_list_pattern", parser->message_list_pattern);
    post.addQueryItem("verify_login_pattern", parser->verify_login_pattern);
    post.addQueryItem("login_parameters", parser->login_parameters);
    post.addQueryItem("login_type", QString().number(parser->login_type));
    post.addQueryItem("charset", parser->charset.toLower());
    post.addQueryItem("thread_list_page_start", QString().number(parser->thread_list_page_start));
    post.addQueryItem("thread_list_page_increment", QString().number(parser->thread_list_page_increment));
    post.addQueryItem("view_thread_page_start", QString().number(parser->view_thread_page_start));
    post.addQueryItem("view_thread_page_increment", QString().number(parser->view_thread_page_increment));
    post.addQueryItem("forum_software", parser->forum_software);
    post.addQueryItem("view_message_path", parser->view_message_path);
    post.addQueryItem("parser_type", QString().number(parser->parser_type));
    post.addQueryItem("posting_path", parser->posting_path);
    post.addQueryItem("posting_subject", parser->posting_subject);
    post.addQueryItem("posting_message", parser->posting_message);
    post.addQueryItem("posting_parameters", parser->posting_parameters);
    post.addQueryItem("posting_hints", parser->posting_hints);
    nam.post(req, post.postData());
}

void SiilihaiProtocol::replySaveParser(QNetworkReply *reply) {
    QString docs = QString().fromUtf8(reply->readAll());
    int id = -1;
    QString msg = QString::null;
    if (reply->error() == QNetworkReply::NoError) {
        QDomDocument doc;
        doc.setContent(docs);
        QDomElement re = doc.firstChildElement("save");
        id = re.firstChildElement("id").text().toInt();
        msg = re.firstChildElement("save_message").text();
    } else {
        qDebug() << Q_FUNC_INFO << "Network error: " << reply->errorString();
        msg = reply->errorString();
    }
    emit saveParserFinished(id, msg);
    reply->deleteLater();
}

void SiilihaiProtocol::addForum(ForumSubscription *sub)
{
    Q_ASSERT(sub->id()==0);
    Q_ASSERT(sub->alias().length()>0);
    Q_ASSERT(sub->forumUrl().isValid());
    QNetworkRequest req(addForumUrl);
    HttpPost post(req, SPOAddForum, clientKey);
    post.addQueryItem("url", sub->forumUrl().toString());
    post.addQueryItem("provider", QString::number(sub->provider()));
    post.addQueryItem("name", sub->alias());
    nam.post(req, post.postData());
}

void SiilihaiProtocol::getForum(QUrl url)
{
    Q_ASSERT(url.isValid());
    QNetworkRequest req(getForumUrl);
    HttpPost post(req, SPOGetForum, clientKey);
    post.addQueryItem("url", url.toString());
    nam.post(req, post.postData());
}

void SiilihaiProtocol::getForum(int forumid)
{
    Q_ASSERT(forumid > 0);
    QNetworkRequest req(getForumUrl);
    HttpPost post(req, SPOGetForum, clientKey);
    post.addQueryItem("forumid", QString::number(forumid));
    nam.post(req, post.postData());
}

void SiilihaiProtocol::replyGetForum(QNetworkReply *reply) {
    QString docs = QString().fromUtf8(reply->readAll());
    if (reply->error() == QNetworkReply::NoError) {
        QDomDocument doc;
        doc.setContent(docs);
        QDomElement re = doc.firstChildElement("forum");
        int id = re.firstChildElement("id").text().toInt();
        int provider = re.firstChildElement("provider").text().toInt();
        QUrl url = QUrl(re.firstChildElement("url").text());
        QString name = re.firstChildElement("name").text();
        QString supportsLoginString = re.firstChildElement("supports_login").text();
        if(id > 0 && provider > 0) {
            ForumSubscription *addedForum = ForumSubscription::newForProvider((ForumSubscription::ForumProvider) provider, 0, true);
            addedForum->setId(id);
            addedForum->setForumUrl(url);
            addedForum->setAlias(name);
            addedForum->setSupportsLogin(supportsLoginString == "True");

            // @todo change protocol & move this to ForumSubscription classes
            if(addedForum->provider() == ForumSubscription::FP_PARSER) {
                int parser = re.firstChildElement("parser").text().toInt();
                qobject_cast<ForumSubscriptionParsed*> (addedForum)->setParserId(parser);
            }

            emit forumGot(addedForum);
            addedForum->deleteLater();
        } else {
            emit forumGot(nullptr);
        }
    } else if(reply->error() == QNetworkReply::ContentNotFoundError) {
        emit forumGot(nullptr); // Not found, this is valid
    } else {
        emit networkError(QString("Network error (this should not happen):\n%1").arg(reply->errorString()));
        emit forumGot(nullptr);
    }
    reply->deleteLater();
}

// This sets the settings (if given) and always gets them
void SiilihaiProtocol::setUserSettings(UserSettings *us) {
    QNetworkRequest req(userSettingsUrl);
    HttpPost post(req, QVariant(), clientKey);
    if(us) {
        post.addQueryItem("sync_enabled", us->syncEnabled() ? "true" : "false");
        req.setAttribute(QNetworkRequest::User, SPOSetUserSettings);
    } else {
        req.setAttribute(QNetworkRequest::User, SPOGetUserSettings);
    }
    nam.post(req, post.postData());
}

void SiilihaiProtocol::replyGetUserSettings(QNetworkReply *reply) {
    QString docs = QString().fromUtf8(reply->readAll());
    UserSettings usettings;
    if (reply->error() == QNetworkReply::NoError) {
        QDomDocument doc;
        doc.setContent(docs);
        QString syncEnabledStr = doc.firstChildElement("usersettings").firstChildElement("sync_enabled").text();
        usettings.setSyncEnabled(!syncEnabledStr.isNull());
    } else {
        qDebug() << Q_FUNC_INFO << "Network error:" << reply->errorString();
        emit userSettingsReceived(false, &usettings);
    }
    emit userSettingsReceived(true, &usettings);
    reply->deleteLater();
}

void SiilihaiProtocol::getUserSettings() {
    setUserSettings(0);
}

void SiilihaiProtocol::sendParserReport(ParserReport *pr) {
    QNetworkRequest req(sendParserReportUrl);
    HttpPost post(req, SPOSendParserReport, clientKey);
    post.addQueryItem("parser_id", QString().number(pr->parserid));
    post.addQueryItem("type", QString().number(pr->type));
    post.addQueryItem("comment", pr->comment);
    nam.post(req, post.postData());
}

void SiilihaiProtocol::setOffline(bool offline)
{
    if (m_offline == offline)
        return;
    m_offline = offline;
    emit offlineChanged(m_offline);
}

void SiilihaiProtocol::replySendParserReport(QNetworkReply *reply) {
    QString docs = QString().fromUtf8(reply->readAll());
    bool success = false;
    if (reply->error() == QNetworkReply::NoError) {
        QDomDocument doc;
        doc.setContent(docs);
        QString successStr = doc.firstChildElement("success").text();
        if (successStr == "true") {
            success = true;
        } else {
            qDebug() << Q_FUNC_INFO << "Network error: " << reply->errorString();
        }
    }
    emit sendParserReportFinished(success);
    reply->deleteLater();
}

// ########### Sync stuff below (move to another file?)

void SiilihaiProtocol::sendThreadData(ForumGroup *grp, QList<ForumMessage*> &fms) {
    QNetworkRequest req(sendThreadDataUrl);
    QDomDocument doc("SiilihaiML");
    QDomElement root = doc.createElement("ThreadData");
    doc.appendChild(root);

    QDomElement forumTag = doc.createElement("forum");
    root.appendChild(forumTag);

    QDomText t = doc.createTextNode(QString().number(grp->subscription()->id()));
    forumTag.appendChild(t);

    QDomElement groupTag = doc.createElement("group");
    root.appendChild(groupTag);

    groupTag.appendChild(doc.createTextNode(grp->id()));

    QDomElement changesetTag = doc.createElement("changeset");
    root.appendChild(changesetTag);

    changesetTag.appendChild(doc.createTextNode(QString().number(grp->changeset())));

    // Sort 'em to threads:
    QMap<ForumThread*, QList<ForumMessage*> > threadedMessages; // Thread id, message
    for (ForumMessage *fm : fms) {
        if (fm->isRead())
            threadedMessages[fm->thread()].append(fm);
    }

    // Send thread data
    QMapIterator<ForumThread*, QList<ForumMessage*> > i(threadedMessages);
    while (i.hasNext()) {
        i.next();
        QDomElement threadTag = doc.createElement("thread");
        threadTag.setAttribute("id", i.key()->id());
        threadTag.setAttribute("changeset", i.key()->changeset());
        threadTag.setAttribute("getmessagescount", i.key()->getMessagesCount());

        for(ForumMessage *fm : i.value()) {
            QDomElement messageTag = doc.createElement("message");
            messageTag.setAttribute("id", fm->id());
            threadTag.appendChild(messageTag);
        }
        root.appendChild(threadTag);
    }

    req.setAttribute(QNetworkRequest::User, SPOSendThreadData);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QString("application/x-www-form-urlencoded"));
    nam.post(req, doc.toByteArray());
}

void SiilihaiProtocol::replySendThreadData(QNetworkReply *reply) {
    bool success = reply->error() == QNetworkReply::NoError;
    if(!success) {
        emit networkError(QString("Network error (this should not happen):\n%1").arg(reply->errorString()));
    }
    reply->deleteLater();
    emit sendThreadDataFinished(success, reply->errorString());
}

void SiilihaiProtocol::downsync(QList<ForumGroup*> &groups) {
    QNetworkRequest req(downsyncUrl);
    QDomDocument doc("SiilihaiML");
    QDomElement root = doc.createElement("ThreadData");
    doc.appendChild(root);
    // Sort them to sub/group structure
    QMap<ForumSubscription*,QList<ForumGroup*> > groupMap;
    for(ForumGroup *grp : groups) {
        groupMap[grp->subscription()].append(grp);
    }
    for(ForumSubscription* sub : groupMap.keys()) {
        QDomElement forumTag = doc.createElement("forum");
        forumTag.setAttribute("id", sub->id());
        root.appendChild(forumTag);

        for(ForumGroup *grp : groupMap.value(sub)) {
            QDomElement groupTag = doc.createElement("group");
            forumTag.appendChild(groupTag);
            groupTag.appendChild(doc.createTextNode(grp->id()));
        }
    }

    req.setAttribute(QNetworkRequest::User, SPOGetThreadData);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QString("application/x-www-form-urlencoded"));
    nam.post(req, doc.toByteArray());
}

void SiilihaiProtocol::replyDownsync(QNetworkReply *reply) {
    QString docs = QString().fromUtf8(reply->readAll());
    QMap<ForumThread, QList<ForumMessage> > downsyncData;
    if (reply->error() == QNetworkReply::NoError) {
        QDomDocument doc;
        doc.setContent(docs);
        QDomElement re = doc.firstChildElement("downsync");
        QDomElement forumElement = re.firstChildElement("forum");
        while(!forumElement.isNull()) {
            int forumid = forumElement.attribute("id").toInt();
            int forumProvider = forumElement.attribute("provider").toInt();
            ForumSubscription *forum = ForumSubscription::newForProvider((ForumSubscription::ForumProvider) forumProvider, 0, true);
            if(forum->provider() == ForumSubscription::FP_PARSER) {
                ForumSubscriptionParsed *forumParsed = qobject_cast<ForumSubscriptionParsed*> (forum);
                int forumparser = forumElement.attribute("id").toInt();
                forumParsed->setParserId(forumparser);
            }
            forum->setId(forumid);
            QDomElement groupElement = forumElement.firstChildElement("group");
            while(!groupElement.isNull()) {
                QString groupid = groupElement.attribute("id");
                ForumGroup group(forum, true);
                group.setId(groupid);
                forum->addGroup(&group);
                QDomElement threadElement = groupElement.firstChildElement("thread");
                while(!threadElement.isNull()) {
                    QString threadid = threadElement.attribute("id");
                    Q_ASSERT(threadid.length()>0);
                    int threadChangeset = threadElement.attribute("changeset").toInt();
                    int threadGetMessagesCount = threadElement.attribute("getmessagescount").toInt();
                    ForumThread thread(&group);
                    thread.setId(threadid);
                    thread.setChangeset(threadChangeset);
                    thread.setGetMessagesCount(threadGetMessagesCount);
                    thread.setName(UNKNOWN_SUBJECT);
                    thread.setOrdernum(999);
                    if(group.contains(thread.id())) {
                        qDebug() << Q_FUNC_INFO
                                 << "Warning: group "
                                 << group.toString()
                                 << "already contains thread with id"
                                 << thread.id()
                                 << "not adding it. Broken parser?";
                    } else {
                        group.addThread(&thread);
                    }
                    emit serverThreadData(&thread);
                    QDomElement messageElement = threadElement.firstChildElement("message");
                    while(!messageElement.isNull()) {
                        QString messageid = messageElement.attribute("id");
                        ForumMessage msg(&thread);
                        msg.setId(messageid);
                        msg.setName(UNKNOWN_SUBJECT);
                        msg.setBody("Please update forum to get message content.");
                        msg.setRead(true, false);
                        msg.setOrdernum(999);
                        thread.addMessage(&msg);
                        emit serverMessageData(&msg);
                        messageElement = messageElement.nextSiblingElement("message");
                    }
                    threadElement = threadElement.nextSiblingElement("thread");
                }
                groupElement = groupElement.nextSiblingElement("group");
            }
            emit downsyncFinishedForForum(forum);
            delete forum;
            forumElement = forumElement.nextSiblingElement("forum");
        }
    } else {
        emit networkError(QString("Network error (this should not happen):\n%1").arg(reply->errorString()));
    }
    reply->deleteLater();
    emit getThreadDataFinished(reply->error() == QNetworkReply::NoError, reply->errorString());
}

void SiilihaiProtocol::getSyncSummary() {
    QNetworkRequest req(syncSummaryUrl);
    HttpPost post(req, SPOGetSyncSummary, clientKey);
    nam.post(req, post.postData());
}

void SiilihaiProtocol::replyGetSyncSummary(QNetworkReply *reply) {
    QString docs = QString().fromUtf8(reply->readAll());
    QList<ForumSubscription*> subs;
    QList<ForumGroup*> grps; // to keep groups in context
    if (reply->error() == QNetworkReply::NoError) {
        QDomDocument doc;
        doc.setContent(docs);
        QDomElement re = doc.firstChildElement("syncsummary");
        QDomElement forumElement = re.firstChildElement("forum");
        while(!forumElement.isNull()) {
            int forumid = forumElement.attribute("id").toInt();
            int forumprovider = forumElement.attribute("provider").toInt();

            ForumSubscription *sub = ForumSubscription::newForProvider((ForumSubscription::ForumProvider) forumprovider, this, true);
            if(sub->provider() == ForumSubscription::FP_PARSER) {
                ForumSubscriptionParsed *forumParsed = qobject_cast<ForumSubscriptionParsed*>(sub);
                int forumparser = forumElement.attribute("parser").toInt();
                forumParsed->setParserId(forumparser);
                Q_ASSERT(forumparser > 0);
            }
            sub->setId(forumid);
            sub->setAlias(forumElement.attribute("alias"));
            sub->setForumUrl(forumElement.attribute("url"));
            sub->setLatestThreads(forumElement.attribute("latest_threads").toInt());
            sub->setLatestMessages(forumElement.attribute("latest_messages").toInt());
            sub->setAuthenticated(forumElement.hasAttribute("authenticated"));
            QDomElement groupElement = forumElement.firstChildElement("group");
            while(!groupElement.isNull()) {
                QString groupid = groupElement.attribute("id");
                int changeset = groupElement.text().toInt();
                ForumGroup *g = new ForumGroup(sub); // I hope qobject's memory management will delete this
                g->setId(groupid);
                g->setChangeset(changeset);
                g->setSubscribed(true);
                grps.append(g);
                sub->addGroup(g);
                groupElement = groupElement.nextSiblingElement("group");
            }
            subs.append(sub);
            forumElement = forumElement.nextSiblingElement("forum");
        }
    } else {
        emit networkError(QString("Network error (this should not happen):\n%1").arg(reply->errorString()));
    }
    emit serverGroupStatus(subs);
    reply->deleteLater();
    for(ForumSubscription *sub : subs) sub->deleteLater();
    subs.clear();
}

bool SiilihaiProtocol::isLoggedIn() {
    return !clientKey.isNull();
}

bool SiilihaiProtocol::offline() const
{
    return m_offline;
}
