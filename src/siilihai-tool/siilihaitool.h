#ifndef SIILIHAITOOL_H
#define SIILIHAITOOL_H

#include <QObject>
#include "siilihai/siilihaiprotocol.h"
#include "siilihai/forumprobe.h"

class ForumSubscription;

class SiilihaiTool : public QObject
{
    Q_OBJECT
public:
    explicit SiilihaiTool(QObject *parent = 0);
    ~SiilihaiTool();

signals:

public slots:
    void listForums();
    void getForum(int id);
    void probe(QUrl url);
private slots:
    void listForumsFinished(QList<ForumSubscription*> forums);
    void forumGot(ForumSubscription* sub);
    void probeResults(ForumSubscription *probedSub);
private:
    void printForum(ForumSubscription *sub);
    SiilihaiProtocol protocol;
    ForumProbe forumProbe;

    QStringList providers;
};

#endif // SIILIHAITOOL_H
