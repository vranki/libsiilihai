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

SiilihaiProtocol::SiilihaiProtocol(QObject *parent) : QObject(parent) {
    operationInProgress = SPONoOp;
    nam.setCookieJar(new QNetworkCookieJar(this));
    forumBeingSubscribed = 0;
    connect(&nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkReply(QNetworkReply*)), Qt::UniqueConnection);
}

SiilihaiProtocol::~SiilihaiProtocol() {
}

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
    //listSubscriptionsUrl = QUrl(baseUrl + "api/subscriptionlist.xml");
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
    QHash<QString, QString> params;
    params.insert("username", user);
    params.insert("password", pass);
    params.insert("action", "login");
    params.insert("clientversion", CLIENT_VERSION);
    loginData = HttpPost::setPostParameters(&req, params);
    req.setAttribute(QNetworkRequest::User, SPOLogin);
    nam.post(req, loginData);
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
        qDebug() << Q_FUNC_INFO << "network error: " << reply->errorString();
    }
    if (ck.length() > 0)
        clientKey = ck;
    emit loginFinished(isLoggedIn(), motd, syncEnabled);
    reply->deleteLater();
}

// Reply is handled in login
void SiilihaiProtocol::registerUser(QString user, QString pass, QString email, bool sync) {
    QNetworkRequest req(registerUrl);
    QHash<QString, QString> params;
    params.insert("username", user);
    params.insert("password", pass);
    params.insert("email", email);
    if(sync)
        params.insert("sync_enabled", "true");
    params.insert("captcha", "earth"); // @todo something smarter
    params.insert("clientversion", CLIENT_VERSION);
    registerData = HttpPost::setPostParameters(&req, params);
    req.setAttribute(QNetworkRequest::User, SPORegisterUser);
    nam.post(req, registerData);
}

void SiilihaiProtocol::listForums() {
    QNetworkRequest req(listForumsUrl);
    QHash<QString, QString> params;
    if (!clientKey.isNull()) {
        params.insert("client_key", clientKey);
    }
    listForumsData = HttpPost::setPostParameters(&req, params);
    req.setAttribute(QNetworkRequest::User, SPOListForums);
    nam.post(req, listForumsData);
}

void SiilihaiProtocol::replyListForums(QNetworkReply *reply) {
    QString docs = QString().fromUtf8(reply->readAll());
    QList<ForumSubscription*> forums;
    if (reply->error() == QNetworkReply::NoError) {
        QDomDocument doc;
        doc.setContent(docs);
        QDomElement n = doc.firstChildElement("parserlist").firstChildElement("parser");
        while (!n.isNull()) {
            // @todo This still uses parsers as backward-compatibility
            ForumParser *parser = XmlSerialization::readParser(n, this);
             if(parser) {
                 ForumSubscription *sub = new ForumSubscription(0, true, ForumSubscription::FP_NONE);
                 sub->setForumId(parser->id());
                 sub->setAlias(parser->name());
                 sub->setForumUrl(parser->forum_url);
                 sub->setSupportsLogin(parser->supportsLogin());
                 forums.append(sub);
             }
            n = n.nextSiblingElement("parser");
        }
    } else {
        qDebug() << Q_FUNC_INFO << "Network error: " << reply->errorString();
    }
    emit(listForumsFinished(forums));
    reply->deleteLater();
}

void SiilihaiProtocol::listRequests() {
    QNetworkRequest req(listRequestsUrl);
    QHash<QString, QString> params;
    if (!clientKey.isNull()) {
        params.insert("client_key", clientKey);
    }
    listRequestsData = HttpPost::setPostParameters(&req, params);
    req.setAttribute(QNetworkRequest::User, SPOListRequests);
    nam.post(req, listRequestsData);
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
    QHash<QString, QString> params;
    params.insert("id", QString().number(id));
    if (!clientKey.isNull()) {
        params.insert("client_key", clientKey);
    }

    getParserData = HttpPost::setPostParameters(&req, params);
    req.setAttribute(QNetworkRequest::User, SPOGetParser);
    nam.post(req, getParserData);
}

void SiilihaiProtocol::replyGetParser(QNetworkReply *reply) {
    QString docs = QString().fromUtf8(reply->readAll());
    ForumParser *parser=0;
    if (reply->error() == QNetworkReply::NoError) {
        QDomDocument doc;
        doc.setContent(docs);
        QDomElement re = doc.firstChildElement("parser");
        parser = XmlSerialization::readParser(re, this);
        parser->update_date = QDate::currentDate();
    } else {
        qDebug() << Q_FUNC_INFO << "Network error: " << reply->errorString();
    }
    emit getParserFinished(parser);
    reply->deleteLater();
    parser->deleteLater();
}

void SiilihaiProtocol::subscribeForum(ForumSubscription *fs, bool unsubscribe) {
    QNetworkRequest req(subscribeForumUrl);
    QHash<QString, QString> params;
    params.insert("forum_id", QString().number(fs->forumId()));
    if (unsubscribe) {
        params.insert("unsubscribe", "yes");
    } else {
        params.insert("url", fs->forumUrl().toString());
        params.insert("provider", QString().number(fs->provider()));
        params.insert("alias", fs->alias());
        params.insert("latest_threads", QString().number(fs->latestThreads()));
        params.insert("latest_messages", QString().number(fs->latestMessages()));
        if(fs->username().length() > 0) {
            params.insert("authenticated", "yes");
        }
    }
    if (!clientKey.isNull()) {
        params.insert("client_key", clientKey);
    }
    subscribeForumData = HttpPost::setPostParameters(&req, params);
    forumBeingSubscribed = fs;
    req.setAttribute(QNetworkRequest::User, SPOSubscribeForum);
    nam.post(req, subscribeForumData);
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

    QDomText t = doc.createTextNode(QString().number(fs->forumId()));
    forumTag.appendChild(t);

    foreach(ForumGroup *g, fs->values()) {
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
    subscribeGroupsData = doc.toByteArray();
    req.setAttribute(QNetworkRequest::User, SPOSubscribeGroups);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QString("application/x-www-form-urlencoded"));
    nam.post(req, subscribeGroupsData);
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
    QHash<QString, QString> params;
    params.insert("id", QString().number(parser->id()));
    params.insert("parser_name", parser->name());
    params.insert("forum_url", parser->forum_url);
    params.insert("parser_status", QString().number(parser->parser_status));
    params.insert("thread_list_path", parser->thread_list_path);
    params.insert("view_thread_path", parser->view_thread_path);
    params.insert("login_path", parser->login_path);
    params.insert("date_format", QString().number(parser->date_format));
    params.insert("group_list_pattern", parser->group_list_pattern);
    params.insert("thread_list_pattern", parser->thread_list_pattern);
    params.insert("message_list_pattern", parser->message_list_pattern);
    params.insert("verify_login_pattern", parser->verify_login_pattern);
    params.insert("login_parameters", parser->login_parameters);
    params.insert("login_type", QString().number(parser->login_type));
    params.insert("charset", parser->charset.toLower());
    params.insert("thread_list_page_start", QString().number(parser->thread_list_page_start));
    params.insert("thread_list_page_increment", QString().number(parser->thread_list_page_increment));
    params.insert("view_thread_page_start", QString().number(parser->view_thread_page_start));
    params.insert("view_thread_page_increment", QString().number(parser->view_thread_page_increment));
    params.insert("forum_software", parser->forum_software);
    params.insert("view_message_path", parser->view_message_path);
    params.insert("parser_type", QString().number(parser->parser_type));
    params.insert("posting_path", parser->posting_path);
    params.insert("posting_subject", parser->posting_subject);
    params.insert("posting_message", parser->posting_message);
    params.insert("posting_parameters", parser->posting_parameters);
    params.insert("posting_hints", parser->posting_hints);

    if (!clientKey.isNull()) {
        params.insert("client_key", clientKey);
    }

    saveParserData = HttpPost::setPostParameters(&req, params);
    req.setAttribute(QNetworkRequest::User, SPOSaveParser);
    nam.post(req, saveParserData);
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
    Q_ASSERT(sub->forumId()==0);
    Q_ASSERT(sub->alias().length()>0);
    Q_ASSERT(sub->forumUrl().isValid());
    QNetworkRequest req(addForumUrl);
    QHash<QString, QString> params;
    params.insert("url", sub->forumUrl().toString());
    params.insert("provider", QString::number(sub->provider()));
    params.insert("name", sub->alias());
    if (!clientKey.isNull()) {
        params.insert("client_key", clientKey);
    }
    req.setAttribute(QNetworkRequest::User, SPOAddForum);
    addForumData = HttpPost::setPostParameters(&req, params);
    nam.post(req, addForumData);
}

void SiilihaiProtocol::getForum(QUrl url)
{
    Q_ASSERT(url.isValid());
    QNetworkRequest req(getForumUrl);
    QHash<QString, QString> params;
    params.insert("url", url.toString());
    if (!clientKey.isNull()) {
        params.insert("client_key", clientKey);
    }
    req.setAttribute(QNetworkRequest::User, SPOGetForum);
    getForumData = HttpPost::setPostParameters(&req, params);
    nam.post(req, getForumData);
}

void SiilihaiProtocol::getForum(int forumid)
{
    Q_ASSERT(forumid > 0);
    QNetworkRequest req(getForumUrl);
    QHash<QString, QString> params;
    params.insert("forumid", QString::number(forumid));
    if (!clientKey.isNull()) {
        params.insert("client_key", clientKey);
    }
    req.setAttribute(QNetworkRequest::User, SPOGetForum);
    getForumData = HttpPost::setPostParameters(&req, params);
    nam.post(req, getForumData);
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
            addedForum->setForumId(id);
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
            emit forumGot(0);
        }
    } else {
        qDebug() << Q_FUNC_INFO << "Network error: " << reply->errorString();
        emit forumGot(0);
    }
    reply->deleteLater();
}

// This sets the settings (if given) and always gets them
void SiilihaiProtocol::setUserSettings(UserSettings *us) {
    QNetworkRequest req(userSettingsUrl);
    QHash<QString, QString> params;
    if(us) {
        if(us->syncEnabled()) {
            params.insert("sync_enabled", "true");
        } else {
            params.insert("sync_enabled", "false");
        }
        if (!clientKey.isNull()) {
            params.insert("client_key", clientKey);
        }
        req.setAttribute(QNetworkRequest::User, SPOSetUserSettings);
    } else {
        req.setAttribute(QNetworkRequest::User, SPOGetUserSettings);
    }
    userSettingsData = HttpPost::setPostParameters(&req, params);
    nam.post(req, userSettingsData);
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
    QHash<QString, QString> params;
    params.insert("parser_id", QString().number(pr->parserid));
    params.insert("type", QString().number(pr->type));
    params.insert("comment", pr->comment);

    if (!clientKey.isNull()) {
        params.insert("client_key", clientKey);
    }
    sendParserReportData = HttpPost::setPostParameters(&req, params);
    req.setAttribute(QNetworkRequest::User, SPOSendParserReport);
    nam.post(req, sendParserReportData);
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

    QDomText t = doc.createTextNode(QString().number(grp->subscription()->forumId()));
    forumTag.appendChild(t);

    QDomElement groupTag = doc.createElement("group");
    root.appendChild(groupTag);

    groupTag.appendChild(doc.createTextNode(grp->id()));

    QDomElement changesetTag = doc.createElement("changeset");
    root.appendChild(changesetTag);

    changesetTag.appendChild(doc.createTextNode(QString().number(grp->changeset())));

    // Sort 'em to threads:
    QMap<ForumThread*, QList<ForumMessage*> > threadedMessages; // Thread id, message
    foreach (ForumMessage *fm, fms) {
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

        foreach(ForumMessage *fm, i.value()) {
            QDomElement messageTag = doc.createElement("message");
            messageTag.setAttribute("id", fm->id());
            threadTag.appendChild(messageTag);
        }
        root.appendChild(threadTag);
    }
    sendThreadDataData = doc.toByteArray();

    req.setAttribute(QNetworkRequest::User, SPOSendThreadData);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QString("application/x-www-form-urlencoded"));
    nam.post(req, sendThreadDataData);
}

void SiilihaiProtocol::replySendThreadData(QNetworkReply *reply) {
    bool success = reply->error() == QNetworkReply::NoError;
    if(!success) {
        qDebug() << Q_FUNC_INFO << "Network error: " << reply->errorString();
    }
    reply->deleteLater();
    emit sendThreadDataFinished(success, reply->errorString());
}

void SiilihaiProtocol::downsync(QList<ForumGroup*> &groups) {
    QNetworkRequest req(downsyncUrl);
    QHash<QString, QString> params;

    QDomDocument doc("SiilihaiML");
    QDomElement root = doc.createElement("ThreadData");
    doc.appendChild(root);
    // Sort them to sub/group structure
    QMap<ForumSubscription*,QList<ForumGroup*> > groupMap;
    foreach(ForumGroup *grp, groups) {
        groupMap[grp->subscription()].append(grp);
    }
    foreach(ForumSubscription* sub, groupMap.keys()) {
        QDomElement forumTag = doc.createElement("forum");
        forumTag.setAttribute("id", sub->forumId());
        root.appendChild(forumTag);

        foreach(ForumGroup *grp, groupMap.value(sub)) {
            QDomElement groupTag = doc.createElement("group");
            forumTag.appendChild(groupTag);
            groupTag.appendChild(doc.createTextNode(grp->id()));
        }
    }
    if (!clientKey.isNull()) {
        params.insert("client_key", clientKey);
    }
    getThreadDataData = doc.toByteArray();
    req.setAttribute(QNetworkRequest::User, SPOGetThreadData);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QString("application/x-www-form-urlencoded"));
    nam.post(req, getThreadDataData);
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
            if(forum->isParsed()) {
                ForumSubscriptionParsed *forumParsed = qobject_cast<ForumSubscriptionParsed*> (forum);
                int forumparser = forumElement.attribute("id").toInt();
                forumParsed->setParserId(forumparser);
            }
            forum->setForumId(forumid);
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
                    group.addThread(&thread);
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
        qDebug() << Q_FUNC_INFO << "Network error: " << reply->error() << reply->errorString();
    }
    reply->deleteLater();
    emit getThreadDataFinished(reply->error() == QNetworkReply::NoError, reply->errorString());
}

void SiilihaiProtocol::getSyncSummary() {
    QNetworkRequest req(syncSummaryUrl);
    QHash<QString, QString> params;

    if (!clientKey.isNull()) {
        params.insert("client_key", clientKey);
    }
    syncSummaryData = HttpPost::setPostParameters(&req, params);
    req.setAttribute(QNetworkRequest::User, SPOGetSyncSummary);
    nam.post(req, syncSummaryData);
}

void SiilihaiProtocol::replyGetSyncSummary(QNetworkReply *reply) {
    QString docs = QString().fromUtf8(reply->readAll());
    QList<ForumSubscription*> subs;
    QList<ForumGroup*> grps; // to keep groups in context
    if (reply->error() == QNetworkReply::NoError) {
        qDebug() << Q_FUNC_INFO << docs;
        QDomDocument doc;
        doc.setContent(docs);
        QDomElement re = doc.firstChildElement("syncsummary");
        QDomElement forumElement = re.firstChildElement("forum");
        while(!forumElement.isNull()) {
            int forumid = forumElement.attribute("id").toInt();
            int forumprovider = forumElement.attribute("provider").toInt();

            ForumSubscription *sub = ForumSubscription::newForProvider((ForumSubscription::ForumProvider) forumprovider, this, true);
            if(sub->isParsed()) {
                ForumSubscriptionParsed *forumParsed = qobject_cast<ForumSubscriptionParsed*>(sub);
                int forumparser = forumElement.attribute("parser").toInt();
                forumParsed->setParserId(forumparser);
                Q_ASSERT(forumparser > 0);
            }
            sub->setForumId(forumid);
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
        emit serverGroupStatus(subs);
    } else {
        qDebug() << Q_FUNC_INFO << "Network error: " << reply->errorString();
    }
    reply->deleteLater();
    foreach(ForumSubscription *sub, subs)
        sub->deleteLater();
    subs.clear();
}

bool SiilihaiProtocol::isLoggedIn() {
    return !clientKey.isNull();
}
