#include "forumdatabase.h"
#include "../forumdata/forumsubscription.h"
#include "../forumdata/forumgroup.h"
#include "../forumdata/forumthread.h"
#include "../forumdata/forummessage.h"

ForumDatabase::ForumDatabase(QObject *parent) : QObject(parent) {}

ForumThread* ForumDatabase::getThread(const int forum, QString groupid, QString threadid) {
    ForumSubscription *fs = value(forum);
    Q_ASSERT(fs);
    ForumGroup *fg = fs->value(groupid);
    Q_ASSERT(fg);
    ForumThread *ft = fg->value(threadid);
    return ft;
}

ForumMessage* ForumDatabase::getMessage(const int forum, QString groupid, QString threadid,
                                        QString messageid) {
    ForumThread *thread = getThread(forum, groupid, threadid);
    if(!thread) {
        qDebug() << Q_FUNC_INFO << "ERROR: Searching for message " << messageid << " in thread " << threadid << " in group "
                 << groupid << " in forum " << forum << " but the thread doesn't exist!!";
        Q_ASSERT(thread);
        return 0;
    }
    return thread->value(messageid);
}

QList<QObject*> ForumDatabase::subscriptions() const
{
    QList<QObject*> allSubscriptions;

    foreach(ForumSubscription *sub, values())
        allSubscriptions.append(qobject_cast<QObject*>(sub));

    return allSubscriptions;
}

#ifdef SANITY_CHECKS
void ForumDatabase::checkSanity() {
    foreach(ForumSubscription *s, values()) {
        int uc = 0;
        foreach(ForumGroup *g, s->values()) {
            if(g->isSubscribed()) {
                int old_uc = uc;
                foreach(ForumThread *t, g->values()) {
                    int tuc=0;
                    foreach(ForumMessage *m, t->values()) {
                        if(!m->isRead()) tuc++;
                    }
                    Q_ASSERT(t->unreadCount() == tuc);
                    uc += tuc;
//                    if(tuc) qDebug() << Q_FUNC_INFO << "Thread" << t->toString()
//                                     << " contains " << t->unreadCount() << "unreads";
                }
                int guc = uc - old_uc;
                if(g->unreadCount() != guc) {
                    qDebug() << Q_FUNC_INFO << "Group " + g->toString() << " reports " << g->unreadCount()
                             << " unreads, i counted " << guc;
                    Q_ASSERT(g->unreadCount() == guc);
                }
            }
        }
        Q_ASSERT(s->unreadCount() == uc);
    }
}
#else
void ForumDatabase::checkSanity() { }
#endif
