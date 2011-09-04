#include "parsermanager.h"
#include "parserdatabase.h"
#include "siilihaiprotocol.h"

ParserManager::ParserManager(QObject *parent, SiilihaiProtocol *pro) : QObject(parent), protocol(pro) {
    parserDatabase = new ParserDatabase(this);
    connect(protocol, SIGNAL(getParserFinished(ForumParser*)), this, SLOT(storeOrUpdateParser(ForumParser*)));
    connect(protocol, SIGNAL(loginFinished(bool,QString,bool)), this, SLOT(loginFinished(bool)));
}

ParserManager::~ParserManager() {
    if(parserDatabase) parserDatabase->storeDatabase();
}

ForumParser *ParserManager::getParser(int id) {
    return parserDatabase->value(id);
}

void ParserManager::uploadParser(ForumParser *parser) {
    protocol->saveParser(parser);
}

void ParserManager::openDatabase(QString filename) {
    parserDatabase->openDatabase(filename);
}

void ParserManager::deleteParser(int id) {
    parserDatabase->remove(id);
}

void ParserManager::storeOrUpdateParser(ForumParser* parser) {
    // Clear parser queue one at a time
    if(!parsersToUpdate.isEmpty() && protocol->isLoggedIn()) {
        updateParser(parsersToUpdate.takeFirst());
    }
    if(!parser) return;
    ForumParser *newParser = parserDatabase->value(parser->id);
    if(!newParser) newParser = new ForumParser(parserDatabase);
    (*newParser) = (*parser);
    Q_ASSERT(newParser != parser);
    Q_ASSERT(newParser->id == parser->id);
    parserDatabase->insert(newParser->id, newParser);
    emit parserUpdated(newParser);
    parser->deleteLater();
}

void ParserManager::updateParser(int id) {
    if(parsersToUpdate.contains(id)) return; // Already queued!

    if(protocol->isLoggedIn()) {
        protocol->getParser(id);
    } else {
        qDebug() << Q_FUNC_INFO << "Not logged in, queuing parser " << id;
        parsersToUpdate.append(id);
    }
}

void ParserManager::loginFinished(bool success) {
    if(success && !parsersToUpdate.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "Logged in, emptying queue ";
        updateParser(parsersToUpdate.takeFirst());
    }
}
