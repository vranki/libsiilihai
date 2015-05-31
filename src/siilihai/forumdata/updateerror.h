#ifndef UPDATEERROR_H
#define UPDATEERROR_H

#include <QObject>

class UpdateError : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString title READ title)
    Q_PROPERTY(QString description READ description)
    Q_PROPERTY(QString technicalData READ technicalData)

public:
    UpdateError();
    UpdateError(QString title, QString description=QString::null, QString technicalData=QString::null);
    QString title() const;
    QString description() const;
    QString technicalData() const;

private:
    QString m_title;
    QString m_description;
    QString m_technicalData;
};

#endif // UPDATEERROR_H
