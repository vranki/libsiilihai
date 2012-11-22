#ifndef PARSERMANAGER_H
#define PARSERMANAGER_H
#include <QObject>
#include <QQueue>
class ParserDatabase;
class ForumParser;
class SiilihaiProtocol;

/**
  * Manages the collection of ForumParsers.
  *
  * Uses ParserDatabase as backend.
  * @see ForumParser
  * @see ParserDatabase
  */
class ParserManager : public QObject
{
    Q_OBJECT
public:
    ParserManager(QObject *parent, SiilihaiProtocol *pro);
    ~ParserManager();
    void openDatabase(QString filename);
    ForumParser *getParser(int id);
    void updateParser(int parserId); // download the parser from server and signal parseravailable
    void uploadParser(ForumParser *parser);
    void deleteParser(int id);
private slots:
    // Called from protocol
    void storeOrUpdateParser(ForumParser* parser); // Called by protocol. Owership does not change
signals:
    void parserUpdated(ForumParser *parser); // Parser was d/l'd from server
private:
    ParserDatabase *parserDatabase;
    SiilihaiProtocol *protocol;
};

#endif // PARSERMANAGER_H
