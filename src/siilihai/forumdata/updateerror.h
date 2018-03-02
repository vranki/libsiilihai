#ifndef UPDATEERROR_H
#define UPDATEERROR_H

#include <QObject>

class UpdateError : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY errorChanged)
    Q_PROPERTY(QString description READ description NOTIFY errorChanged)
    Q_PROPERTY(QString technicalData READ technicalData NOTIFY errorChanged)
public:
    UpdateError();
    UpdateError(const UpdateError &o);
    UpdateError(QString title, QString description=QString(), QString technicalData=QString());
    QString title() const;
    QString description() const;
    QString technicalData() const;

signals:
    void errorChanged(); // Never changes

private:
    QString m_title;
    QString m_description;
    QString m_technicalData;
};

#endif // UPDATEERROR_H
