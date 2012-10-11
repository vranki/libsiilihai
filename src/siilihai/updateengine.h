#ifndef UPDATEENGINE_H
#define UPDATEENGINE_H

#include <QObject>
#include <QObject>
#include <QQueue>
#include <QNetworkAccessManager>
#include <QAuthenticator>

class ForumSubscription;
class ForumGroup;
class ForumThread;
class ForumMessage;
class ForumDatabase;
class CredentialsRequest;

class UpdateEngine : public QObject
{
    Q_OBJECT

public:
    // Remember to sync with stateNames in .cpp
    enum UpdateEngineState {
        PES_UNKNOWN=0,
        PES_ENGINE_NOT_READY,
        PES_IDLE,
        PES_UPDATING,
        PES_ERROR
    };

    UpdateEngine(QObject *parent, ForumDatabase *fd);
    virtual ~UpdateEngine();
    virtual void setSubscription(ForumSubscription *fs);
    virtual void updateGroupList();
    virtual void updateForum(bool force=false);
    virtual void updateThread(ForumThread *thread, bool force=false);
    UpdateEngine::UpdateEngineState state();
    ForumSubscription* subscription() const;
    QNetworkAccessManager *networkAccessManager();
public slots:
    virtual void cancelOperation();
    virtual void credentialsEntered(CredentialsRequest* cr);
signals:
    // Emitted if initially group list was empty but new groups were found.
    void groupListChanged(ForumSubscription *forum);
    void forumUpdated(ForumSubscription *forum);
    void statusChanged(ForumSubscription *forum, float progress);
    void updateFailure(ForumSubscription *forum, QString message);
    void getHttpAuthentication(ForumSubscription *fsub, QAuthenticator *authenticator); // Asynchronous
    void getForumAuthentication(ForumSubscription *fsub); // Asynchronous
    void loginFinished(ForumSubscription *sub, bool success);
    // Caution: engine's subscription may be null!
    void stateChanged(UpdateEngine::UpdateEngineState newState, UpdateEngine::UpdateEngineState oldState);
    void updateForumSubscription(ForumSubscription *fsub); // Used to request protocol to update subscription

protected slots:
    void listMessagesFinished(QList<ForumMessage*> &messages, ForumThread *thread, bool moreAvailable);
    void listGroupsFinished(QList<ForumGroup*> &groups);
    void listThreadsFinished(QList<ForumThread*> &threads, ForumGroup *group);
    void networkFailure(QString message);
    void loginFinishedSlot(ForumSubscription *sub, bool success);
    void subscriptionDeleted();

protected:
    void updateNextChangedGroup();
    void updateNextChangedThread();
    void updateCurrentProgress();
    void setState(UpdateEngineState newState);
    virtual void requestCredentials();

    // Do the actual work
    virtual void doUpdateForum()=0;
    virtual void doUpdateGroup(ForumGroup *group)=0;
    virtual void doUpdateThread(ForumThread *thread)=0;
protected:
    bool updateAll;
    bool forceUpdate; // Update even if no changes
    bool requestingCredentials;
    ForumDatabase *fdb;
    QQueue<ForumThread*> threadsToUpdateQueue;
    bool updateWhenEngineReady;
    QNetworkAccessManager nam;
private:
    ForumSubscription *fsubscription;
    UpdateEngineState currentState;
};

#endif // UPDATEENGINE_H
