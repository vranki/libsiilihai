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
#include "../forumdata/forumgroup.h"
#include "../forumdata/forumthread.h"
#include "../forumdata/forummessage.h"
#include "../forumdatabase/forumdatabase.h"
#include "../forumdata/forumsubscription.h"
#include "../credentialsrequest.h"
#include "parsermanager.h"


ParserEngine::ParserEngine(ForumDatabase *fd, QObject *parent, ParserManager *pm) :
    UpdateEngine(parent, fd), session(this, &nam), parserManager(pm) {
    connect(&session, SIGNAL(listGroupsFinished(QList<ForumGroup*>&, ForumSubscription *)), this, SLOT(listGroupsFinished(QList<ForumGroup*>&, ForumSubscription *)));
    connect(&session, SIGNAL(listThreadsFinished(QList<ForumThread*>&, ForumGroup*)), this, SLOT(listThreadsFinished(QList<ForumThread*>&, ForumGroup*)));
    connect(&session, SIGNAL(listMessagesFinished(QList<ForumMessage*>&, ForumThread*, bool)),
            this, SLOT(listMessagesFinished(QList<ForumMessage*>&, ForumThread*, bool)));
    connect(&session, SIGNAL(networkFailure(QString)), this, SLOT(networkFailure(QString)));
    connect(&session, SIGNAL(getHttpAuthentication(ForumSubscription*, QAuthenticator*)), this, SIGNAL(getHttpAuthentication(ForumSubscription*,QAuthenticator*)));
    connect(&session, SIGNAL(loginFinished(ForumSubscription *,bool)), this, SLOT(loginFinishedSlot(ForumSubscription *,bool)));
    connect(parserManager, SIGNAL(parserUpdated(ForumParser*)), this, SLOT(parserUpdated(ForumParser*)));
    connect(this, SIGNAL(stateChanged(UpdateEngine::UpdateEngineState,UpdateEngine::UpdateEngineState)),
            this, SLOT(updateParserIfError(UpdateEngine::UpdateEngineState,UpdateEngine::UpdateEngineState)));
    currentParser = 0;
    sessionInitialized = false;
    updatingParser = false;
}

ParserEngine::~ParserEngine() {
}

void ParserEngine::setParser(ForumParser *fp) {
    currentParser = fp;
    if(fp) {
        if(subscription() && !updatingParser && state()==PES_ENGINE_NOT_READY)  {
            setState(PES_IDLE);
        }
    } else {
        if(!updatingParser)
            setState(PES_ENGINE_NOT_READY);
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
    if(updateParser) {
        parserManager->updateParser(subscriptionParsed()->parser());
        updatingParser = true;
        setState(PES_ENGINE_NOT_READY);
    } else {
        setState(PES_IDLE);
    }
}

ForumParser *ParserEngine::parser() const {
    return currentParser;
}

void ParserEngine::parserUpdated(ForumParser *p) {
    Q_ASSERT(p);
    if(subscriptionParsed()->parser() == p->id()) {
        qDebug() << Q_FUNC_INFO;
        updatingParser = false;
        setParser(p);
    }
}

void ParserEngine::updateParserIfError(UpdateEngine::UpdateEngineState newState, UpdateEngine::UpdateEngineState oldState) {
    if(newState == PES_ERROR) {
        parserManager->updateParser(subscriptionParsed()->parser());
    }
}

void ParserEngine::updateThread(ForumThread *thread, bool force) {
    initSession();
    UpdateEngine::updateThread(thread, force);
}

void ParserEngine::cancelOperation() {
    updateAll = false;
    updateCanceled = true;
    session.cancelOperation();
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

void ParserEngine::credentialsEntered(CredentialsRequest* cr) {
    if(cr->credentialType==CredentialsRequest::SH_CREDENTIAL_HTTP) {
        session.authenticationReceived();
    }
    UpdateEngine::credentialsEntered(cr);
}


void ParserEngine::doUpdateGroup(ForumGroup *group) {
    session.listThreads(group);
}

void ParserEngine::doUpdateForum()
{
    initSession();
    session.listGroups();
}

void ParserEngine::doUpdateThread(ForumThread *thread)
{
    initSession();
    session.listMessages(thread);
}

void ParserEngine::initSession()
{
    if (!sessionInitialized) {
        session.initialize(currentParser, subscription());
        sessionInitialized = true;
    }
}

ForumSubscriptionParsed *ParserEngine::subscriptionParsed() const
{
    return qobject_cast<ForumSubscriptionParsed*>(subscription());
}
