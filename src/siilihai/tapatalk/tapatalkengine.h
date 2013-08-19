#ifndef TAPATALKENGINE_H
#define TAPATALKENGINE_H
#include "../updateengine.h"

class ForumSubscriptionTapaTalk;
class QDomElement;
class QDomDocument;
/**
 * @brief The TapaTalkEngine class implements a UpdateEngine for
 * TapaTalk forum protocol.
 *
 * @todo Protect against invalid data in responses. Currently we
 * assume server always returns correct data and we may crash/assert
 * if something unexpected happens.
 */
class TapaTalkEngine : public UpdateEngine
{
    Q_OBJECT

    enum TapaTalkOperation {
        TTO_None=0, TTO_ListGroups, TTO_UpdateGroup, TTO_UpdateThread, TTO_Probe, TTO_Login
    };

public:
    explicit TapaTalkEngine(ForumDatabase *fd, QObject *parent);
    virtual void setSubscription(ForumSubscription *fs);
    virtual void probeUrl(QUrl url);

signals:
    void urlProbeResults(ForumSubscription *sub);
public slots:

protected:
    virtual void doUpdateForum();
    virtual void doUpdateGroup(ForumGroup *group);
    virtual void doUpdateThread(ForumThread *thread);
private slots:
    void networkReply(QNetworkReply *reply);
protected:
    virtual void requestCredentials();
private:
    void updateCurrentThreadPage(); // Get next 50 messages in current thread

    void replyProbe(QNetworkReply *reply);
    void replyListGroups(QNetworkReply *reply);
    void replyUpdateGroup(QNetworkReply *reply);
    void replyUpdateThread(QNetworkReply *reply);
    void replyLogin(QNetworkReply *reply);
    QString getValueFromStruct(QDomElement dataValueElement, QString name);
    void getGroups(QDomElement dataValueElement, QList<ForumGroup *> *grps);
    void getChildGroups(QDomElement dataValueElement, QList<ForumGroup *> *grps);

    void getThreads(QDomElement dataValueElement, QList<ForumThread *> *threads);
    void getMessages(QDomElement dataValueElement, QList<ForumMessage *> *messages);

    bool loginIfNeeded(); // True if it is needed first
    void createMethodCall(QDomDocument &doc, QString method, QList<QPair<QString, QString> > &params);
    QDomElement findMemberValueElement(QDomElement dataValueElement, QString memberName);
    QString valueElementToString(QDomElement valueElement); // Value should be something like <value><int>29</int></value>
    ForumSubscriptionTapaTalk *subscriptionTapaTalk() const;
    void convertBodyToHtml(ForumMessage *msg); // convert [url=][/url] etc to real html

    QString connectorUrl;
    ForumGroup *groupBeingUpdated;
    ForumThread *threadBeingUpdated;
    bool loggedIn;
    // TapaTalk sends only 50 messages per request, so we must use "pages" to update longer threads
    int currentMessagePage;
    QList<ForumMessage*> messages;
};

#endif // TAPATALKENGINE_H
