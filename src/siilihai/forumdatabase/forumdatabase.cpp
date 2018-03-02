#include "forumdatabase.h"
#include "../forumdata/forumsubscription.h"
#include "../forumdata/forumgroup.h"
#include "../forumdata/forumthread.h"
#include "../forumdata/forummessage.h"
#include "../forumdata/grouplistmodel.h"

ForumDatabase::ForumDatabase(QObject *parent) : QObject(parent)
//  , m_groupListModel(this)
{
    connect(this, SIGNAL(subscriptionFound(ForumSubscription*)), this, SIGNAL(subscriptionsChanged()));
    connect(this, SIGNAL(subscriptionRemoved(ForumSubscription*)), this, SIGNAL(subscriptionsChanged()));
}

ForumDatabase::ForumDatabase::~ForumDatabase()
{
    qDebug() << Q_FUNC_INFO;
    resetDatabase();
}

void ForumDatabase::resetDatabase(){
    while (!isEmpty()) {
        deleteSubscription(last());
    }
    clear();
    emit subscriptionsChanged();
    checkSanity();
}

bool ForumDatabase::addSubscription(ForumSubscription *fs){
    insert(fs->id(), fs);
    emit subscriptionsChanged();
    emit subscriptionFound(fs);
    checkSanity();
#ifdef SANITY_CHECKS
    for(ForumGroup *g : fs->values())
        connect(g, SIGNAL(threadAdded(ForumThread*)), this, SLOT(checkSanity()));
#endif
    return true;
}

void ForumDatabase::deleteSubscription(ForumSubscription *sub) {
    // Delete the groups first.
    for(ForumGroup *g : *sub)
        sub->removeGroup(g, false, false);
    // Then delete the whole sub
    removeAll(sub);
    emit subscriptionsChanged();
    emit subscriptionRemoved(sub);
    sub->deleteLater();
}
bool ForumDatabase::contains(int id) {
    return findById(id);
}

bool ForumDatabase::contains(ForumSubscription *other)
{
    for(auto sub : *this) {
        if(sub == other) return true;
    }
    return false;
}

ForumSubscription *ForumDatabase::findById(int id)
{
    for(auto sub : *this) {
        if(sub->id() == id) return sub;
    }
    return nullptr;
}

ForumThread* ForumDatabase::getThread(const int forum, QString groupid, QString threadid) {
    ForumSubscription *fs = findById(forum);
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
        return nullptr;
    }
    return thread->value(messageid);
}

QList<QObject*> ForumDatabase::subscriptions()
{
    QList<QObject*> allSubscriptions;

    for(ForumSubscription *sub : *this)
        allSubscriptions.append(qobject_cast<QObject*>(sub));

    return allSubscriptions;
}
/*
QObject *ForumDatabase::groupListModel()
{
    return qobject_cast<QObject*> (&m_groupListModel);
}
*/
#ifdef SANITY_CHECKS
void ForumDatabase::checkSanity() {
    for(ForumSubscription *s : values()) {
        int uc = 0;
        for(ForumGroup *g : s->values()) {
            if(g->isSubscribed()) {
                int old_uc = uc;
                for(ForumThread *t : g->values()) {
                    int tuc=0;
                    for(ForumMessage *m : t->values()) {
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
