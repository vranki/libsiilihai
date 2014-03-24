#ifndef TAPATALKENGINE_H
#define TAPATALKENGINE_H
#include "../updateengine.h"

class ForumSubscriptionTapaTalk;
class QDomElement;
class QDomDocument;


// Helper struct to aid getting full name of a group
struct GroupHierarchyItem {
    QString name;
    QString parentId;
};

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
        TTO_None=0, TTO_ListGroups, TTO_UpdateGroup, TTO_UpdateThread, TTO_Probe, TTO_Login, TTO_PostMessage
    };

public:
    explicit TapaTalkEngine(ForumDatabase *fd, QObject *parent);
    virtual void setSubscription(ForumSubscription *fs);
    virtual void probeUrl(QUrl url);
    virtual bool supportsPosting();
    virtual QString convertDate(QString &date);
public slots:
    virtual bool postMessage(ForumGroup *grp, ForumThread *thr, QString subject, QString body);
signals:
    void urlProbeResults(ForumSubscription *sub);

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
    void protocolErrorDetected(); // Print output & cause networkFailure()

    void replyProbe(QNetworkReply *reply);
    void replyListGroups(QNetworkReply *reply);
    void replyUpdateGroup(QNetworkReply *reply);
    void replyUpdateThread(QNetworkReply *reply);
    void replyLogin(QNetworkReply *reply);
    void replyPost(QNetworkReply *reply);
    QString getValueFromStruct(QDomElement dataValueElement, QString name);
    void getGroups(QDomElement dataValueElement, QList<ForumGroup *> *grps, QMap<QString, GroupHierarchyItem> &groupHierarchy);
    void getChildGroups(QDomElement dataValueElement, QList<ForumGroup *> *grps, QMap<QString, GroupHierarchyItem> &groupHierarchy);

    void getThreads(QDomElement dataValueElement, QList<ForumThread *> *threads);
    void getMessages(QDomElement dataValueElement, QList<ForumMessage *> *messages);

    bool loginIfNeeded(); // True if it is needed first
    void createMethodCall(QDomDocument &doc, QString method, QList<QPair<QString, QString> > &params);

    // Creates full groupname ("Rootgroup / SubGroup / name")
    QString groupHierarchyString(QMap<QString, GroupHierarchyItem> &groupHierarchy, QString id);
    // Creates full groupnames for all groups in grps
    void fixGroupNames(QList<ForumGroup *> *grps, QMap<QString, GroupHierarchyItem> &groupHierarchy);

    QDomElement findMemberValueElement(QDomElement dataValueElement, QString memberName);
    QString valueElementToString(QDomElement valueElement); // Value should be something like <value><int>29</int></value>
    ForumSubscriptionTapaTalk *subscriptionTapaTalk() const;
    void convertBodyToHtml(ForumMessage *msg); // convert [url=][/url] etc to real html
    QString connectorUrl;
    bool loggedIn;
    // TapaTalk sends only 50 messages per request, so we must use "pages" to update longer threads
    int currentMessagePage;
    QList<ForumMessage*> messages;
};

#endif // TAPATALKENGINE_H
