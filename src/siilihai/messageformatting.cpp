#include "messageformatting.h"
/*
MessageFormatting::MessageFormatting() {
}

MessageFormatting::~MessageFormatting() {
}*/

QString MessageFormatting::sanitize(const QString &txt) {
    QString s = stripHtml(txt);
    s.replace('\n', "");
    s.replace(QRegExp("\\s+"), " ");
    return s;
}

QString MessageFormatting::stripHtml(const QString &txt) {
    QRegExp tagRe = QRegExp("<.*>");
    tagRe.setMinimal(true);
    QString newTxt = txt;
    newTxt.replace(tagRe, "");
    // @todo Smarter way to do this
    newTxt.replace("&amp;", "&");
    newTxt.replace("&quot;", "\"");
    newTxt.replace("&lt;", "<");
    newTxt.replace("&gt;", ">");
    newTxt.replace("&nbsp;", " ");
    newTxt.replace("&#63;", "?");
    newTxt.replace("&#8230;", "'");
    newTxt.replace("&#8220;", "\"");
    newTxt.replace("&#8221;", "\"");
    newTxt.replace("&#8217;", "'");
    newTxt.replace("&ouml;", "ö");
    newTxt.replace("&auml;", "ä");
    newTxt.replace("&bull;", "*");
    return newTxt;
}

QString MessageFormatting::replaceCharacters(const QString &txt) {
    QString newTxt = txt;
    newTxt.replace("&", "&amp;");
    newTxt.replace("\"", "&quot;");
    newTxt.replace("<", "&lt;");
    newTxt.replace(">", "&gt;");
    return newTxt;
}
