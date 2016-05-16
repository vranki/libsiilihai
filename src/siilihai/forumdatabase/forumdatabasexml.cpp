#include <QDomDocument>

#include "forumdatabasexml.h"
#include "../forumdata/forumsubscription.h"
#include "../forumdata/forumgroup.h"
#include "../forumdata/forumthread.h"
#include "../forumdata/forummessage.h"

#include "../xmlserialization.h"

#ifdef SANITY_CHECKS
#include "../forumdata/forumgroup.h"
#endif

ForumDatabaseXml::ForumDatabaseXml(QObject *parent) :
    ForumDatabase(parent) {
    resetDatabase();
    m_databaseFileName = QString::null;
}

void ForumDatabaseXml::resetDatabase(){
    m_unsaved = false;
    m_loaded = false;
    for(ForumSubscription *sub : *this)
        deleteSubscription(sub);
    clear();
    emit subscriptionsChanged();
    checkSanity();
}

int ForumDatabaseXml::schemaVersion(){
    return 11;
}

bool ForumDatabaseXml::openDatabase(QString filename, bool loadContent) {
    m_databaseFileName = filename;
    QFile databaseFile(m_databaseFileName);
    if (!databaseFile.open(QIODevice::ReadOnly))
        return false;
    bool success = openDatabase(&databaseFile, loadContent);
    databaseFile.close();
    return success;
}

bool ForumDatabaseXml::openDatabase(QIODevice *source, bool loadContent) {
    if(loadContent) {
        QDomDocument doc("Siilihai_ForumDatabase");
        if(!doc.setContent(source)) return false;
        QDomElement docElem = doc.documentElement();
        if(docElem.tagName() != "forumdatabase") return false;

        QDomElement subscriptionElement = docElem.firstChildElement(SUB_SUBSCRIPTION);
        while(!subscriptionElement.isNull()) {
            ForumSubscription *sub = XmlSerialization::readSubscription(subscriptionElement, this);
            if(sub) {
                // Fix unread counts
                sub->incrementUnreadCount(-sub->unreadCount());
                Q_ASSERT(sub->unreadCount()==0);
                for(ForumGroup *grp : sub->values()) {
                    grp->incrementUnreadCount(-grp->unreadCount());
                    Q_ASSERT(grp->unreadCount()==0);
                    if(grp->isSubscribed()) {
                        for(ForumThread *thr : grp->values()) {
                            thr->incrementUnreadCount(-thr->unreadCount());
                            Q_ASSERT(thr->unreadCount()==0);
                            for(ForumMessage *msg : thr->values()) {
                                if(!msg->isRead()) {
                                    thr->incrementUnreadCount(1);
                                    grp->incrementUnreadCount(1);
                                    sub->incrementUnreadCount(1);
                                }
                            }
                        }
                    }
                }
                insert(sub->id(), sub);
                emit subscriptionFound(sub);
            }
            subscriptionElement = subscriptionElement.nextSiblingElement(SUB_SUBSCRIPTION);
        }
    }
    m_loaded = true;
    checkSanity();
    emit subscriptionsChanged();
    return true;
}

bool ForumDatabaseXml::isStored(){
    return !m_unsaved;
}

bool ForumDatabaseXml::addSubscription(ForumSubscription *fs){
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

void ForumDatabaseXml::deleteSubscription(ForumSubscription *sub) {
    // Delete the groups first.
    for(ForumGroup *g : sub->values())
        sub->removeGroup(g, false, false);
    // Then delete the whole sub
    removeAll(sub);
    emit subscriptionsChanged();
    emit subscriptionRemoved(sub);
    sub->deleteLater();
}

bool ForumDatabaseXml::storeDatabase(){
    checkSanity();
    if(m_databaseFileName.isNull()) {
        qDebug() << Q_FUNC_INFO << "no file open!";
        return false;
    }
    QDomDocument doc("Siilihai_ForumDatabase");
    QDomElement root = doc.createElement("forumdatabase");
    root.setAttribute("schema_version", QString::number(schemaVersion()));
    doc.appendChild(root);

    for(ForumSubscription *sub : *this) {
        XmlSerialization::serialize(sub, root, doc);
    }

    QFile databaseFile(m_databaseFileName);
    if (!databaseFile.open(QIODevice::WriteOnly)) {
        qDebug() << Q_FUNC_INFO << "can't open " << m_databaseFileName << " for writing";
        return false;
    }
    if(databaseFile.write(doc.toByteArray()) <0) {
        databaseFile.close();
        return false;
    }
    databaseFile.close();
    emit databaseStored();
    return true;
}

