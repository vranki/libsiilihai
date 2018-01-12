#ifndef SIILIHAITOOL_H
#define SIILIHAITOOL_H

#include <QObject>
#include "siilihai/siilihaiprotocol.h"
#include "siilihai/forumprobe.h"
#include "siilihai/updateengine.h"
#include "siilihai/parser/parsermanager.h"
#include "siilihai/parser/forumparser.h"

// Support for ancient Qt versions (Travis)
#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
#define qInfo       qDebug
#define qWarning    qDebug
#define qFatal      qDebug
#define qCritical   qDebug
#endif

class ForumSubscription;

class SiilihaiTool : public QObject
{
    Q_OBJECT
public:
    explicit SiilihaiTool(QObject *parent = 0);
    ~SiilihaiTool();
    void setNoServer(bool ns);
    void setForumId(const int id);
    void setForumUrl(const QUrl url);
    bool success();
signals:

public slots:
    void listForums();
    void getForum(const int id);
    void probe();
    void listGroups();
    void listThreads(QString groupId);
    void listMessages(QString groupId, QString threadId);
    void updateForum();

private slots:
    void listForumsFinished(QList<ForumSubscription*> forums);
    void forumGot(ForumSubscription* sub);
    void probeResults(ForumSubscription *probedSub);
    void groupListChanged(ForumSubscription* sub);
    void threadsChanged();
    void messagesChanged();
    void engineStateChanged(UpdateEngine *engine, UpdateEngine::UpdateEngineState newState, UpdateEngine::UpdateEngineState oldState);
    void parserUpdated(ForumParser *newParser);
    void progressReport(ForumSubscription *forum, float progress);


private:
    void printForum(ForumSubscription *sub);
    void printForumErrors(ForumSubscription *sub);
    void copyToCurrentSubscription(const ForumSubscription *tempSub);
    void performCommand();

    SiilihaiProtocol protocol;
    ForumProbe forumProbe;
    ParserManager parserManager;
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
    int m_forumId;
    QUrl m_forumUrl;
    bool m_success;
};

#endif // SIILIHAITOOL_H
