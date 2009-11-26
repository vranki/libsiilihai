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
	void serverGroupStatus(QList<ForumGroup> grps);
	// void serverThreadStatus(ForumThread thread);
	void threadChanged(ForumThread &thread);
	void uploadNextGroup(bool success);
	void downloadNextGroup(bool success);
	void sendThreadDataFinished(bool success);
	void serverThreadData(ForumThread &thread);
	void serverMessageData(ForumMessage &message);

signals:
	void syncFinished(bool success);

private:
	void processGroups();

	ForumDatabase &fdb;
	SiilihaiProtocol &protocol;
	QList<ForumGroup> serversGroups;
	QList<ForumThread> serversThreads;
	QQueue<ForumGroup> groupsToUpload;
	QQueue<ForumGroup> groupsToDownload;
	QSet<ForumThread> changedThreads;
	QSet<int> forumsToUpload;
	QQueue<ForumMessage> messagesToUpload;
};

#endif /* SYNCMASTER_H_ */
