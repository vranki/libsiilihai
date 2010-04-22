#ifndef FORUMMESSAGE_H_
#define FORUMMESSAGE_H_
#include <QString>
#include <QObject>
#include "forumthread.h"

class ForumMessage : public QObject {
    Q_OBJECT
public:
    virtual ~ForumMessage();
    ForumMessage(ForumThread *thr);
    ForumMessage& operator=(const ForumMessage& o);
    ForumMessage(const ForumMessage& o);

    bool isSane() const;
    QString toString() const;
    ForumThread* thread() const;
    QString id() const;
    int ordernum() const;
    QString url() const;
    QString subject() const;
    QString author() const;
    QString lastchange() const;
    QString body() const;
    bool read() const;

    void setId(QString nid);
    void setOrdernum(int nod);
    void setUrl(QString nurl);
    void setSubject(QString ns);
    void setAuthor(QString na);
    void setLastchange(QString nlc);
    void setBody(QString nb);
    void setRead(bool nr);
    void setThread(ForumThread *nt);
private:
    QString _id;
    int _ordernum;
    QString _url;
    QString _subject;
    QString _author;
    QString _lastchange;
    QString _body;
    bool _read;
    ForumThread *_thread;
};

#endif /* FORUMMESSAGE_H_ */
