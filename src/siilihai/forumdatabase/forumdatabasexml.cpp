#include <QDomDocument>

#include "forumdatabasexml.h"
#include "../forumdata/forumsubscription.h"
#include "../xmlserialization.h"

#ifdef SANITY_CHECKS
#include "../forumdata/forumgroup.h"
#endif

ForumDatabaseXml::ForumDatabaseXml(QObject *parent) :
    ForumDatabase(parent) {
    resetDatabase();
    databaseFileName = QString::null;
}

void ForumDatabaseXml::resetDatabase(){
    unsaved = false;
    loaded = false;
    foreach(ForumSubscription *sub, values())
        deleteSubscription(sub);
    clear();
    checkSanity();
}

int ForumDatabaseXml::schemaVersion(){
    return 11;
}

bool ForumDatabaseXml::openDatabase(QString filename, bool loadContent) {
    databaseFileName = filename;
    QFile databaseFile(databaseFileName);
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
                insert(sub->forumId(), sub);
                emit subscriptionFound(sub);
            }
            subscriptionElement = subscriptionElement.nextSiblingElement(SUB_SUBSCRIPTION);
        }
    }
    loaded = true;
    checkSanity();
    return true;
}

bool ForumDatabaseXml::isStored(){
    return !unsaved;
}

bool ForumDatabaseXml::addSubscription(ForumSubscription *fs){
    insert(fs->forumId(), fs);
    emit subscriptionFound(fs);
    checkSanity();
#ifdef SANITY_CHECKS
    foreach(ForumGroup *g, fs->values())
        connect(g, SIGNAL(threadAdded(ForumThread*)), this, SLOT(checkSanity()));
#endif
    return true;
}

void ForumDatabaseXml::deleteSubscription(ForumSubscription *sub){
    remove(sub->forumId());
    emit subscriptionRemoved(sub);
    sub->deleteLater();
}

bool ForumDatabaseXml::storeDatabase(){
    checkSanity();
    if(databaseFileName.isNull()) {
        qDebug() << Q_FUNC_INFO << "no file open!";
        return false;
    }
    QDomDocument doc("Siilihai_ForumDatabase");
    QDomElement root = doc.createElement("forumdatabase");
    root.setAttribute("schema_version", QString::number(schemaVersion()));
    doc.appendChild(root);

    foreach(ForumSubscription *sub, values()) {
        XmlSerialization::serialize(sub, root, doc);
    }

    QFile databaseFile(databaseFileName);
    if (!databaseFile.open(QIODevice::WriteOnly)) {
        qDebug() << Q_FUNC_INFO << "can't open " << databaseFileName << " for writing";
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

