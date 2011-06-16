/* This file is part of libSiilihai.

    libSiilihai is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libSiilihai is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with libSiilihai.  If not, see <http://www.gnu.org/licenses/>. */
#ifndef FORUMMESSAGE_H_
#define FORUMMESSAGE_H_
#include <QString>
#include <QObject>
#include "forumdataitem.h"

class ForumThread;

/**
  * Represents a single message in a thread
  */
class ForumMessage : public ForumDataItem {
    Q_OBJECT
public:
    virtual ~ForumMessage();
    ForumMessage(ForumThread *thr, bool temp=true);
    void copyFrom(ForumMessage * o);
    bool operator<(const ForumMessage &o);
    bool isSane() const;
    virtual QString toString() const;
    ForumThread* thread() const;
    int ordernum() const;
    QString url() const;
    QString author() const;
    QString body() const;
    bool isRead() const;
    void setOrdernum(int nod);
    void setUrl(QString nurl);
    void setAuthor(QString na);
    void setBody(QString nb);
    void setRead(bool nr, bool affectsParents=true);
    bool isTemp();
    virtual void markToBeUpdated(bool toBe=true);
signals:
    void changed(ForumMessage * fm);
    void markedRead(ForumMessage * fm, bool read);
protected:
    virtual void emitChanged();
    virtual void emitUnreadCountChanged();
private:
    int _ordernum;
    QString _url;
    QString _author;
    QString _body;
    bool _read;
    ForumThread *_thread;
    bool _temp;
};

#endif /* FORUMMESSAGE_H_ */
