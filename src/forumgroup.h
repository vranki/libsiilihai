#ifndef FORUMGROUP_H_
#define FORUMGROUP_H_
#include <QString>
#include <QObject>
#include "forumsubscription.h"

class ForumGroup : public QObject {
    Q_OBJECT

public:
    ForumGroup(ForumSubscription *sub);
    virtual ~ForumGroup();
    ForumGroup& operator=(const ForumGroup&);
    ForumGroup(const ForumGroup&);
    QString toString() const;
    bool isSane() const;
    ForumSubscription *subscription() const;
    QString name() const;
    QString id() const;
    QString lastchange() const;
    bool subscribed() const;
    int changeset() const;

    // void setSubscription(ForumSubscription *sub);
    void setName(QString name);
    void setId(QString id);
    void setLastchange(QString lc);
    void setSubscribed(bool s);
    void setChangeset(int cs);
private:
    ForumSubscription *_subscription;
    QString _name;
    QString _id;
    QString _lastchange;
    bool _subscribed;
    int _changeset;
};

#endif
