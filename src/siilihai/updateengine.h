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
#ifndef UPDATEENGINE_H
#define UPDATEENGINE_H

#include <QObject>
#include <QObject>
#include <QQueue>
#include <QNetworkAccessManager>
#include <QAuthenticator>
#include <QUrl>

class ForumSubscription;
class ForumGroup;
class ForumThread;
class ForumMessage;
class ForumDatabase;
class CredentialsRequest;
class ForumDatabase;
class ParserManager;

/**
  * Common parent class for update engines. Handles updating a forum's data (threads, messages, etc).
  *
  * State diagram:
  *
  * UES_ENGINE_NOT_READY -> REQUESTING_CREDENTIALS                   <-> IDLE <-> UPDATING
  *                                    '-->----------------------->------^ ^- ERROR <-'
  * @see ForumDatabase
  *
  * @todo pool network requests and cancel them properly.
  */

class UpdateEngine : public QObject
{
    Q_OBJECT

public:
    // Remember to sync with stateNames in .cpp
    enum UpdateEngineState {
        UES_UNKNOWN=0,
        UES_ENGINE_NOT_READY,
        UES_IDLE,
        UES_UPDATING,
        UES_ERROR
    };
    // fd can be null
    UpdateEngine(QObject *parent, ForumDatabase *fd);
    virtual ~UpdateEngine();
    static UpdateEngine* newForSubscription(ForumSubscription *fs, ForumDatabase *fdb, ParserManager *pm);
    virtual void setSubscription(ForumSubscription *fs);

    // These are the main update functions called by UI. Call only for IDLE engine.
    virtual void updateGroupList();
    virtual void updateForum(bool force=false, bool subscribeNewGroups=false);
    virtual void updateGroup(ForumGroup *group, bool force=false, bool onlyGroup=false); // onlygroup = only update thread list, no threads
    virtual void updateThread(ForumThread *thread, bool force=false, bool onlyThread=false); // onlythread = only update this thread, no more

    virtual bool supportsPosting(); // Returns true if this engine can post new threads & messages
    virtual QString convertDate(QString &date); // Convert date to human-readable format
    QString stateName(UpdateEngineState state);
    UpdateEngine::UpdateEngineState state();
    ForumSubscription* subscription() const;
    QNetworkAccessManager *networkAccessManager();
    // @todo add function to reset login state (error, u/p changed or something)
    virtual void probeUrl(QUrl url)=0;

public slots:
    virtual void cancelOperation();
    virtual void credentialsEntered(CredentialsRequest* cr);
    virtual bool postMessage(ForumGroup *grp, ForumThread *thr, QString subject, QString body);

signals:
    // Emitted if initially group list was empty but new groups were found.
    void groupListChanged(ForumSubscription *forum);
    void forumUpdated(ForumSubscription *forum);
    void progressReport(ForumSubscription *forum, float progress);
    // void updateFailure(ForumSubscription *forum, QString message); // Non-fatal, causes a error dialog
    void getHttpAuthentication(ForumSubscription *fsub, QAuthenticator *authenticator); // Asynchronous
    void getForumAuthentication(ForumSubscription *fsub); // Asynchronous
    void loginFinished(ForumSubscription *sub, bool success);
    // Caution: engine's subscription may be null!
    void stateChanged(UpdateEngine *engine, UpdateEngine::UpdateEngineState newState, UpdateEngine::UpdateEngineState oldState);
    void updateForumSubscription(ForumSubscription *fsub); // Used to request protocol to update subscription
    void messagePosted(ForumSubscription *sub);
    void urlProbeResults(ForumSubscription *sub);

protected slots:
    void listMessagesFinished(QList<ForumMessage*> &messages, ForumThread *thread, bool moreAvailable);
    void listGroupsFinished(QList<ForumGroup*> &groups, ForumSubscription *updatedSub); // These have subscription() as 0
    void listThreadsFinished(QList<ForumThread*> &threads, ForumGroup *group); // These have group() as 0
    void networkFailure(QString message); // Critical error causing state to change to ERROR
    void loginFinishedSlot(ForumSubscription *sub, bool success);
    void subscriptionDeleted();

protected:
    void updateNextChangedGroup();
    void updateNextChangedThread();
    void updateCurrentProgress();
    void setState(UpdateEngineState newState);
    virtual void requestCredentials();
    void continueUpdate(); // Start/continue update which was paused by authentication etc

    // Do the actual work
    virtual void doUpdateForum()=0;
    virtual void doUpdateGroup(ForumGroup *group)=0;
    virtual void doUpdateThread(ForumThread *thread)=0;
    virtual QString engineTypeName()=0; // Engine type as human readable

protected:
    bool forceUpdate; // Update even if no changes
    bool updateOnlyThread; // Only update one thread
    bool updateOnlyGroup; // Only list threads in group, don't list messages
    bool updateOnlyGroups; // Only list groups, don't update them
    bool m_subscribeNewGroups; // Subscribe to all new found groups (useful for testing)
    bool requestingCredentials;
    bool updateCanceled;
    int authenticationRetries; // How many times authentication has been tried
    ForumDatabase *fdb;
    QUrl apiUrl; // Url of the API, without / at end. Usage optional.
    QQueue<ForumThread*> threadsToUpdateQueue;
    bool updateWhenEngineReady;
    QNetworkAccessManager nam;

    ForumGroup *groupBeingUpdated; // Can be null if updating only thread
    ForumThread *threadBeingUpdated; // Must be null if not updating a thread

private:
    ForumSubscription *fsubscription;
    UpdateEngineState currentState;
};

#endif // UPDATEENGINE_H
