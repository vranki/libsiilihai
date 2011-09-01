#ifndef PARSERMANAGER_H
#define PARSERMANAGER_H
#include <QObject>
#include <QQueue>
class ParserDatabase;
class ForumParser;
class SiilihaiProtocol;

class ParserManager : public QObject
{
    Q_OBJECT
public:
    ParserManager(QObject *parent, SiilihaiProtocol *pro);
    ~ParserManager();
    void openDatabase(QString filename);
    ForumParser *getParser(int id);
    void updateParser(int id); // download the parser from server and signal parseravailable
    void uploadParser(ForumParser *parser);
    void deleteParser(int id);
private slots:
    // Called from protocol
    void storeOrUpdateParser(ForumParser* parser); // Owership does not change
    void loginFinished(bool success);
signals:
    void parserUpdated(ForumParser *parser); // Parser was d/l'd from server
private:
    ParserDatabase *parserDatabase;
    SiilihaiProtocol *protocol;
    QQueue<int> parsersToUpdate; // Waiting for login
};

#endif // PARSERMANAGER_H
