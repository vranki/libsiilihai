#ifndef SIILIHAITOOL_H
#define SIILIHAITOOL_H

#include <QObject>
#include "siilihai/siilihaiprotocol.h"
#include "siilihai/forumprobe.h"
#include "siilihai/updateengine.h"

class ForumSubscription;

class SiilihaiTool : public QObject
{
    Q_OBJECT
public:
    explicit SiilihaiTool(QObject *parent = 0);
    ~SiilihaiTool();
    void setNoServer(bool ns);
signals:

public slots:
    void listForums();
    void getForum(int id);
    void probe(QUrl url);
    void listGroups(QUrl url);
    void listThreads(QUrl url, QString groupId);
    void listMessages(QUrl url, QString groupId, QString threadId);

private slots:
    void listForumsFinished(QList<ForumSubscription*> forums);
    void forumGot(ForumSubscription* sub);
    void probeResults(ForumSubscription *probedSub);
    void groupListChanged(ForumSubscription* sub);
    void threadsChanged();
    void messagesChanged();
    void engineStateChanged(UpdateEngine *engine, UpdateEngine::UpdateEngineState newState, UpdateEngine::UpdateEngineState oldState);

private:
    void printForum(ForumSubscription *sub);

    SiilihaiProtocol protocol;
    ForumProbe forumProbe;
    QStringList providers;
    ForumSubscription *m_currentSubscription;
    QString command;
    UpdateEngine *updateEngine;
    bool m_noServer; // Don't use server when probing
    bool m_groupListReceived, m_threadListReceived, m_messageListReceived;
    QString m_groupId;
    ForumGroup *m_groupBeingUpdated;
    QString m_threadId;
    ForumThread *m_threadBeingUpdated;
};

#endif // SIILIHAITOOL_H
