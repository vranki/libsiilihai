#include "messageformatting.h"

MessageFormatting::MessageFormatting() {
}

MessageFormatting::~MessageFormatting() {
}

QString MessageFormatting::sanitize(QString &txt) {
    QString s = stripHtml(txt);
    s.replace('\n', "");
    s.replace(QRegExp("\\s+"), " ");
    return s;
}

QString MessageFormatting::stripHtml(QString &txt) {
    QRegExp tagRe = QRegExp("<.*>");
    tagRe.setMinimal(true);
    txt.replace(tagRe, "");
    // @todo Smarter way to do this
    txt.replace("&amp;", "&");
    txt.replace("&quot;", "\"");
    txt.replace("&lt;", "<");
    txt.replace("&gt;", ">");
    txt.replace("&nbsp;", " ");
    txt.replace("&#63;", "?");
    txt.replace("&#8230;", "'");
    txt.replace("&#8220;", "\"");
    txt.replace("&#8221;", "\"");
    txt.replace("&#8217;", "'");
    return txt;
}

QString MessageFormatting::replaceCharacters(QString &txt) {
    txt.replace("&", "&amp;");
    txt.replace("\"", "&quot;");
    txt.replace("<", "&lt;");
    txt.replace(">", "&gt;");
    return txt;
}
