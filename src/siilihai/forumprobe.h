#ifndef FORUMPROBE_H
#define FORUMPROBE_H

#include <QObject>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include "siilihaiprotocol.h"

#include "forumdata/forumsubscription.h"
/**
 * @brief The ForumProbe class probes for forum info on url or id
 *
 * use probeUrl() with id or url. ProbeResults will be emitted.
 */
#define FORUM_TYPE_COUNT 2

class ForumProbe : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isProbing READ isProbing NOTIFY isProbingChanged)

public:
    explicit ForumProbe(QObject *parent, SiilihaiProtocol *proto);
    void probeUrl(QUrl url, bool noServer=false);
    void probeUrl(int id, bool noServer=false); // Noserver for not using sh server
    bool isProbing() const;

signals:
    void probeResults(ForumSubscription *probedSub);
    void isProbingChanged(bool isProbing);

private slots:
    void finishedSlot(QNetworkReply* reply);
    void forumGot(ForumSubscription *sub);
    void engineProbeResults(ForumSubscription *sub);

private:
    QString getTitle(QString &html);
    void probeNextType();

    QNetworkAccessManager nam;
    SiilihaiProtocol *m_protocol;
    QUrl url;
    UpdateEngine *currentEngine;
    ForumSubscription *probedSub;
    unsigned int typesProbed; // How many different forum types have been probed
    bool m_isProbing;
};

#endif // FORUMPROBE_H
