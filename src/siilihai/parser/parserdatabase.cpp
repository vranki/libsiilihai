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
#include "parserdatabase.h"
#include "../xmlserialization.h"
#include <QDomDocument>

ParserDatabase::ParserDatabase(QObject *parent) : QObject(parent) {
}

ParserDatabase::~ParserDatabase() {
}

bool ParserDatabase::openDatabase(QString filename) {
    databaseFileName = filename;
    QFile databaseFile(databaseFileName);
    if (!databaseFile.open(QIODevice::ReadOnly))
        return false;
    bool success = openDatabase(&databaseFile);
    databaseFile.close();
    return success;
}

bool ParserDatabase::openDatabase(QIODevice *source) {
    QDomDocument doc("Siilihai_ParserDatabase");
    if(!doc.setContent(source)) return false;
    QDomElement docElem = doc.documentElement();
    if(docElem.tagName() != "parserdatabase") return false;
    QDomElement parserElement = docElem.firstChildElement("parser");
    while(!parserElement.isNull()) {
        ForumParser *parser = XmlSerialization::readParser(parserElement, this);
        if(parser) {
            insert(parser->id(), parser);
        }
        parserElement = parserElement.nextSiblingElement("parser");
    }
    return true;
}

bool ParserDatabase::storeDatabase() {
    if(databaseFileName.isNull()) {
        return false;
    }
    QDomDocument doc("Siilihai_ParserDatabase");
    QDomElement root = doc.createElement("parserdatabase");
    doc.appendChild(root);

    foreach(ForumParser *p, values()) {
        XmlSerialization::serialize(p, root, doc);
    }

    QFile databaseFile(databaseFileName);
    if (!databaseFile.open(QIODevice::WriteOnly)) {
        return false;
    }
    if(databaseFile.write(doc.toByteArray()) <0) {
        databaseFile.close();
        return false;
    }
    databaseFile.close();
    return true;
}
