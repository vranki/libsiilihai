#ifndef FORUMDATAITEM_H
#define FORUMDATAITEM_H

#include <QObject>
#include "updateableitem.h"

#define NEEDS_UPDATE "ITEM_NEEDS_UPDATE"
#define UNKNOWN_SUBJECT "?"

/**
  * Pure virtual class containing common functionality for
  * groups, threads and messages.
  */
class ForumDataItem : public QObject, public UpdateableItem {
    Q_OBJECT
public:
    ForumDataItem(QObject * parent);
    virtual QString toString() const=0;
    void setName(QString name);
    void setId(QString id);
    QString name() const;
    QString id() const;
    int unreadCount() const;
    virtual void incrementUnreadCount(int urc);
    void setLastchange(QString nlc);
    QString lastchange() const;
    void commitChanges();
protected:
    virtual void emitChanged()=0;
    virtual void emitUnreadCountChanged()=0;
    bool _propertiesChanged;
private:
    Q_DISABLE_COPY(ForumDataItem);
    QString _name;
    QString _id;
    QString _lastchange;
    int _unreadCount;
};

#endif // FORUMDATAITEM_H
