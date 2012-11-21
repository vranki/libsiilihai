#include "parsermanager.h"
#include "parserdatabase.h"
#include "../siilihaiprotocol.h"

ParserManager::ParserManager(QObject *parent, SiilihaiProtocol *pro) : QObject(parent), protocol(pro) {
    parserDatabase = new ParserDatabase(this);
    connect(protocol, SIGNAL(getParserFinished(ForumParser*)), this, SLOT(storeOrUpdateParser(ForumParser*)));
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
}

void ParserManager::updateParser(int id) {
    protocol->getParser(id);
}
