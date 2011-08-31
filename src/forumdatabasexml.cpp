#include <QDomDocument>

#include "forumdatabasexml.h"
#include "forumsubscription.h"
#include "xmlserialization.h"

ForumDatabaseXml::ForumDatabaseXml(QObject *parent) :
    ForumDatabase(parent) {
    resetDatabase();
    databaseFileName = QString::null;
}

void ForumDatabaseXml::resetDatabase(){
    unsaved = false;
    loaded = false;
    clear();
}

int ForumDatabaseXml::schemaVersion(){
    return 10;
}

bool ForumDatabaseXml::openDatabase(QString filename) {
    databaseFileName = filename;
    QFile databaseFile(databaseFileName);
    if (!databaseFile.open(QIODevice::ReadOnly))
        return false;
    bool success = openDatabase(&databaseFile);
    databaseFile.close();
    return success;
}

bool ForumDatabaseXml::openDatabase(QIODevice *source) {
    QDomDocument doc("Siilihai_ForumDatabase");
    if(!doc.setContent(source)) return false;
    QDomElement docElem = doc.documentElement();
    if(docElem.tagName() != "forumdatabase") return false;
    QDomElement subscriptionElement = docElem.firstChildElement(SUB_SUBSCRIPTION);
    while(!subscriptionElement.isNull()) {
        qDebug() << qPrintable(subscriptionElement.tagName()); // the node really is an element.
        ForumSubscription *sub = XmlSerialization::readSubscription(subscriptionElement, this);
        if(sub) {
            insert(sub->parser(), sub);
            emit subscriptionFound(sub);
        }
        subscriptionElement = subscriptionElement.nextSiblingElement(SUB_SUBSCRIPTION);
    }
    loaded = true;
    return true;
}

bool ForumDatabaseXml::isStored(){
    return !unsaved;
}

bool ForumDatabaseXml::addSubscription(ForumSubscription *fs){
    insert(fs->parser(), fs);
    emit subscriptionFound(fs);
    return true;
}

void ForumDatabaseXml::deleteSubscription(ForumSubscription *sub){
    remove(sub->parser());
    emit subscriptionRemoved(sub);
    sub->deleteLater();
}

bool ForumDatabaseXml::storeDatabase(){
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
