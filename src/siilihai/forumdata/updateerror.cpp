#include "updateerror.h"

UpdateError::UpdateError(): QObject()
{
}

UpdateError::UpdateError(const UpdateError &o) : QObject()
{
   m_title = o.title();
   m_description = o.description();
   m_technicalData = o.technicalData();
}

UpdateError::UpdateError(QString title, QString description, QString technicalData): QObject(),
    m_title(title), m_description(description), m_technicalData(technicalData)
{
}

QString UpdateError::title() const
{
    return m_title;
}

QString UpdateError::description() const
{
    return m_description;
}

QString UpdateError::technicalData() const
{
    return m_technicalData;
}
