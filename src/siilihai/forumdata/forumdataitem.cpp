#include "forumdataitem.h"
#include "../messageformatting.h"

ForumDataItem::ForumDataItem(QObject * parent) : QObject(parent), UpdateableItem()
{
    _name = _id = _displayName = QString::null;
    _unreadCount = 0;
    _propertiesChanged = false;
}

void ForumDataItem::setId(const QString &nid) {
  if(nid == _id) return;
  _id = nid;
  _propertiesChanged = true;
}

const QString & ForumDataItem::id() const {
    return _id;
}

void ForumDataItem::setName(QString const &name) {
  if(name == _name) return;
  _name = name;
  _propertiesChanged = true;
  _displayName = QString::null;
}

const QString & ForumDataItem::name() const {
    return _name;
}

const QString &ForumDataItem::displayName() {
    if(_displayName.isNull()) {
        _displayName = MessageFormatting::stripHtml(MessageFormatting::sanitize(name()));
    }
    return _displayName;
}

QString ForumDataItem::lastchange() const {
    return _lastchange;
}

void ForumDataItem::setLastchange(QString nlc) {
    if(nlc==_lastchange) return;
    _lastchange = nlc;
    _propertiesChanged = true;
}

void ForumDataItem::commitChanges() {
    if(_propertiesChanged) emitChanged();
    _propertiesChanged = false;
}

int ForumDataItem::unreadCount() const {
    return _unreadCount;
}

void ForumDataItem::incrementUnreadCount(const int &urc) {
    if(!urc) return;
    _unreadCount += urc;
    Q_ASSERT(_unreadCount >= 0);
    emitUnreadCountChanged();
}
