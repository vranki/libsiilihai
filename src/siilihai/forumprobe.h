#ifndef FORUMPROBE_H
#define FORUMPROBE_H

#include <QObject>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include "siilihaiprotocol.h"

#include "forumdata/forumsubscription.h"

class ForumProbe : public QObject
{
    Q_OBJECT
public:
    explicit ForumProbe(QObject *parent, SiilihaiProtocol &proto);
    void probeUrl(QUrl url);
    void probeUrl(int id);

signals:
    void probeResults(ForumSubscription *probedSub);

private slots:
    void finishedSlot(QNetworkReply* reply);
    void forumGot(ForumSubscription *sub);
    void engineProbeResults(ForumSubscription *sub);
private:
    QString getTitle(QString &html);

    QNetworkAccessManager nam;
    SiilihaiProtocol &protocol;
    QUrl url;
    UpdateEngine *currentEngine;
    ForumSubscription *probedSub;
};

#endif // FORUMPROBE_H
