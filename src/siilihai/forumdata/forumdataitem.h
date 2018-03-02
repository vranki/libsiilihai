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
    void setName(const QString &name);
    void setId(const QString &id);
    const QString &name() const;
    virtual const QString &displayName(); // Get human-readable name in plain text
    const QString &id() const;
    int unreadCount() const;
    virtual void incrementUnreadCount(const int &urc);
    void setLastChange(QString nlc);
    QString lastChange() const;
    void commitChanges();
protected:
    virtual void emitChanged()=0;
    virtual void emitUnreadCountChanged()=0;
    bool _propertiesChanged;
    QString _displayName; // Update if values changed
private:
    Q_DISABLE_COPY(ForumDataItem)
    QString _name;
    QString _id;
    QString _lastchange;
    int _unreadCount;
};

#endif // FORUMDATAITEM_H
