#include "forumdataitem.h"

ForumDataItem::ForumDataItem(QObject * parent) : QObject(parent)
{
    _name = _id = QString::null;
    _unreadCount = 0;
}


void ForumDataItem::setId(QString nid) {
  if(nid == _id) return;
  _id = nid ;
  emitChanged();
}

QString ForumDataItem::id() const {
    return _id;
}

void ForumDataItem::setName(QString name) {
  if(name == _name) return;
  _name = name;
  emitChanged();
}

QString ForumDataItem::name() const {
    return _name;
}

QString ForumDataItem::lastchange() const {
    return _lastchange;
}

void ForumDataItem::setLastchange(QString nlc) {
    if(nlc==_lastchange) return;
    _lastchange = nlc ;
    emitChanged();
}

int ForumDataItem::unreadCount() const {
    return _unreadCount;
}

void ForumDataItem::incrementUnreadCount(int urc) {
    if(!urc) return;
    _unreadCount += urc;
    // Q_ASSERT(_unreadCount >= 0);
    emitUnreadCountChanged();
}
