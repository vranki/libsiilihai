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
#ifndef SYNCMASTER_H_
#define SYNCMASTER_H_
#include <QObject>
#include <QList>
#include <QQueue>
#include "forumdatabase/forumdatabase.h"
#include "siilihaiprotocol.h"

class ForumSubscription;
class ForumGroup;
class ForumThread;

/**
  * Handles synchronization of reader state with siilihai.com server.
  * Uses SiilihaiProtocol for actual communication to server.
  *
  * @see SiilihaiProtocol
  */
class SyncMaster : public QObject {
    Q_OBJECT

public:
    SyncMaster(QObject *parent, ForumDatabase &fd, SiilihaiProtocol &prot);
    virtual ~SyncMaster();
public slots:
    /**
     * Downloads state from server (done in startup)
     */
    void startSync();
    /**
     * Uploads state to server (done in end)
     */
    void endSync();
    void cancel();
    void endSyncSingleGroup(ForumGroup *group);
    void serverGroupStatus(const QList<ForumSubscription *> &subs);
    void threadChanged(ForumThread *thread);
    void sendThreadDataFinished(bool success, QString message);
    void serverThreadData(const ForumThread *thread);
    void serverMessageData(const ForumMessage *message);
    void getThreadDataFinished(bool success, QString message);
    void subscribeGroupsFinished(bool success);
    void downsyncFinishedForForum(ForumSubscription *fs);
signals:
    void syncFinished(bool success, QString message);
    void syncFinishedFor(ForumSubscription *fs);
    void syncProgress(float progress, QString message);
private:
    void processSubscriptions();
    void processGroups();
    bool canceled;
    ForumDatabase &fdb;
    SiilihaiProtocol &protocol;
    QList<ForumGroup*> serversGroups;
    QList<ForumThread*> serversThreads;
    QQueue<ForumGroup*> groupsToUpload;
    QList<ForumGroup*> groupsToDownload;
    QSet<ForumThread*> changedThreads;
    QQueue<ForumSubscription*> forumsToUpload;
    QQueue<ForumMessage*> messagesToUpload;
    unsigned int errorCount;
    int maxGroupCount;
};

#endif /* SYNCMASTER_H_ */
