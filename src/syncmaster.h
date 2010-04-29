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
#include <QMap>
#include "forumdatabase.h"
#include "forumgroup.h"
#include "forumthread.h"
#include "siilihaiprotocol.h"

class SyncMaster : public QObject {
	Q_OBJECT

public:
	SyncMaster(QObject *parent, ForumDatabase &fd, SiilihaiProtocol &prot);
	virtual ~SyncMaster();
	void startSync();
	void endSync();
public slots:
        void serverGroupStatus(QList<ForumGroup> &grps);
	void threadChanged(ForumThread *thread);
	void sendThreadDataFinished(bool success);
	void serverThreadData(ForumThread *thread);
	void serverMessageData(ForumMessage *message);
        void getThreadDataFinished(bool success);
signals:
	void syncFinished(bool success);

private:
	void processGroups();

	ForumDatabase &fdb;
	SiilihaiProtocol &protocol;
	QList<ForumGroup*> serversGroups;
	QList<ForumThread*> serversThreads;
	QQueue<ForumGroup*> groupsToUpload;
	QQueue<ForumGroup*> groupsToDownload;
	QSet<ForumThread*> changedThreads;
	QSet<ForumSubscription*> forumsToUpload;
	QQueue<ForumMessage*> messagesToUpload;
};

#endif /* SYNCMASTER_H_ */
