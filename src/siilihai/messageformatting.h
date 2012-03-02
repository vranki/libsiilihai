#ifndef MESSAGEFORMATTING_H_
#define MESSAGEFORMATTING_H_

#include <QString>
#include <QRegExp>

class MessageFormatting {
public:/*
    MessageFormatting();
    virtual ~MessageFormatting();*/
    static QString stripHtml(const QString &txt);
    static QString sanitize(const QString &txt); // strip html and newlines
    static QString replaceCharacters(const QString &txt); // Convert < -> &lt; etc.
};

#endif /* MESSAGEFORMATTING_H_ */
