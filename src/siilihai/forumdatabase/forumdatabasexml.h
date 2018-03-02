#ifndef FORUMDATABASEXML_H
#define FORUMDATABASEXML_H

#include <QIODevice>
#include <QString>
#include <QFile>
#include "forumdatabase.h"

class ForumDatabaseXml : public ForumDatabase
{
    Q_OBJECT
public:
    explicit ForumDatabaseXml(QObject *parent = 0);
    bool openDatabase(QIODevice *source, bool loadContent=true);
    bool openDatabase(QString filename, bool loadContent=true);

    virtual int schemaVersion() override;
    virtual bool isStored() override;
    virtual void resetDatabase() override;

public slots:
    virtual bool storeDatabase() override;

private:
    bool m_unsaved, m_loaded;
    QString m_databaseFileName;

};

#endif // FORUMDATABASEXML_H
