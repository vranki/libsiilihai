#include "parsermanager.h"
#include "parserdatabase.h"
#include "../siilihaiprotocol.h"

ParserManager::ParserManager(QObject *parent, SiilihaiProtocol *pro) :
    QObject(parent)
  , parserDatabase(new ParserDatabase(this))
  , protocol(pro)
{
    connect(protocol, &SiilihaiProtocol::getParserFinished, this, &ParserManager::storeOrUpdateParser);
    connect(protocol, &SiilihaiProtocol::offlineChanged, this, &ParserManager::offlineChanged);
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
    if(!parser) return;
    ForumParser *newParser = parserDatabase->value(parser->id());
    if(!newParser) newParser = new ForumParser(parserDatabase);
    (*newParser) = (*parser);
    Q_ASSERT(newParser != parser);
    Q_ASSERT(newParser->id() == parser->id());
    parserDatabase->insert(newParser->id(), newParser);
    emit parserUpdated(newParser);
    getNextParser();
}

void ParserManager::offlineChanged(bool newOffline) {
    if(!newOffline) getNextParser();
}

void ParserManager::getNextParser() {
    if(m_parserUpdateQueue.isEmpty()) return;
    updateParser(m_parserUpdateQueue.takeFirst());
}

void ParserManager::updateParser(int parserId) {
    if(!protocol->offline()) {
        protocol->getParser(parserId);
    } else {
        if(!m_parserUpdateQueue.contains(parserId))
            m_parserUpdateQueue.append(parserId);
    }
}
