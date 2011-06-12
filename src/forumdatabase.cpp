#include "forumdatabase.h"
#include "forumsubscription.h"
#include "forumgroup.h"
#include "forumthread.h"

ForumDatabase::ForumDatabase(QObject *parent) :
    QObject(parent)
{
}

ForumThread* ForumDatabase::getThread(const int forum, QString groupid, QString threadid) {
    ForumSubscription *fs = value(forum);
    Q_ASSERT(fs);
    ForumGroup *fg = fs->value(groupid);
    Q_ASSERT(fg);
    return fg->value(threadid);
}

ForumMessage* ForumDatabase::getMessage(const int forum, QString groupid,
                                        QString threadid, QString messageid) {
    ForumThread *thread = getThread(forum, groupid, threadid);
    Q_ASSERT(thread);
    if(!thread) return 0;
    return thread->value(messageid);
}
