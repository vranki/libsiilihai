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

#include "patternmatcher.h"

PatternMatcher::PatternMatcher(QObject *parent, bool emitStatus) :
    QObject(parent) {
    patternSet = false;
    es = emitStatus;
}

PatternMatcher::~PatternMatcher() {

}

void PatternMatcher::setPattern(QString &pattern) {
    patternTokens = tokenizePattern(pattern);
    /*
qDebug() << "Converted pattern " << pattern << " to "
            << patternTokens.size() << " tokens.";
                        */
    patternSet = true;
}

bool PatternMatcher::isPatternSet() {
    return patternSet;
}

QList<QString> PatternMatcher::tokenizePattern(QString pattern) {
    QList<QString> patternTokens;
    QString part;
    while (pattern.length() > 0) {
        if (pattern.indexOf('%') >= 0) {
            part = pattern.left(pattern.indexOf('%'));
        } else {
            part = pattern;
        }
        if (part.length() > 0) {
            patternTokens.append(part);
        }
        pattern = pattern.right(pattern.length() - part.length());
        if (pattern.length() < 2 && pattern.indexOf('%') >= 0) {
            qDebug() << "invalid pattern!!";
            return QList<QString> ();
        }
        if (pattern.indexOf('%') >= 0) {
            part = pattern.left(2);
            pattern = pattern.right(pattern.length() - 2);
            patternTokens.append(part);
        }
    }
    if (patternTokens.size() > 0) {
        if (isTag(patternTokens[patternTokens.length() - 1]) || isTag(
                    patternTokens[0])) {
            qDebug() << "Warning: last or first token can't be tag!!";
            return QList<QString> ();
        }
        for (int i = 0; i < patternTokens.length() - 1; i++) {
            if (isTag(patternTokens[i]) && isTag(patternTokens[i + 1])) {
                qDebug() << "Warning: can't have two tokens after eachother!!";
                return QList<QString> ();
            }
        }
    }
    return patternTokens;
}

bool PatternMatcher::isTag(QString &tag) {
    if (tag.length() == 2 && tag[0] == '%' && QChar(tag[1]).isLetterOrNumber())
        return true;
    return false;
}

bool PatternMatcher::isNumberTag(QString &tag) {
    if (isTag(tag)) {
        if (QChar(tag[1]).isUpper())
            return true;
    }
    return false;
}

QString PatternMatcher::numberize(QString &txt) {
    QString result = "";
    bool numberStarted = false;
    for (int i = 0; i < txt.length(); i++) {
        if (QChar(txt[i]).isDigit()) {
            result.append(txt[i]);
            numberStarted = true;
        } else {
            if (numberStarted)
                i = txt.length();
        }
    }
    if (result.length() == 0)
        qDebug() << "Warning! Unable to convert string " << txt
                 << " to numbers!";

    return result;
}

QList<QHash<QString, QString> > PatternMatcher::findMatches(QString &html) {
    QList<QHash<QString, QString> > matches;
    QHash<QString, QString> matchHash;

    int pos = 0;
    int htmllength = html.length();
    PatternMatchType type = PMTNoMatch;
    if (es)
        emit dataMatchingStart(html);
    /*
    if (pt[1] == 'w') { // Only accepting whitespace now
        int matchStart = pos;
        while (pos < htmllength && !html.at(pos).isPrint()) {
            pos++;
        }
        emit dataMatched(matchStart, html.mid(matchStart, pos
                - matchStart), PMTWhitespace);
    }
*/

    while (pos < htmllength && patternTokens.length() > 0) {
        for (int n = 0; n < patternTokens.length() && pos < htmllength; n++) {
            QString pt = patternTokens[n];
            QString nextPt = QString::null;

            if(n < patternTokens.length()-1)
                nextPt = patternTokens[n+1];

            if (isTag(pt)) { // It's a tag like %a
                if (n == patternTokens.length()) {
                    qDebug() << "Panic!! not enough tokens!!";
                }
                //				qDebug() << "Looking for token " << pt << " ending at "
                //						<< nextToken << " at " << html.right(htmllength - pos);
                int matchPos = html.indexOf(nextPt, pos);

                if (matchPos < 0) {
                    if (es)
                        emit dataMatched(pos, html.mid(pos, htmllength - pos),
                                         PMTNoMatch);
                    pos = htmllength;
                } else {
                    if (pt[1] == 'i') {
                        type = PMTIgnored;
                    } else {
                        QString match = html.mid(pos, matchPos - pos);

                        if (isNumberTag(pt)) {
                            match = numberize(match);
                        }
                        //										qDebug() << "tag " << pt << ":" << match;
                        pt = pt.toLower();
                        matchHash[pt] = match.trimmed();
                        type = PMTTag;
                    }
                    if (es)
                        emit dataMatched(pos, html.mid(pos, matchPos - pos),
                                         type);
                    pos = matchPos;
                }
            } else { // NOT tag, just random text
                //				qDebug() << "Looking for text " << pt << " at " << html.right(
                //						htmllength - pos);
                int matchPos = html.indexOf(pt, pos);
                if (matchPos < 0) { // didn't find - skip to end
                    if (es)
                        emit dataMatched(pos, html.mid(pos, htmllength - pos),
                                         PMTNoMatch);
                    pos = htmllength;
                } else { // Match found
                    if (es) {
                        emit dataMatched(pos, html.mid(pos, matchPos - pos),
                                         PMTNoMatch);
                        emit dataMatched(matchPos, html.mid(matchPos,
                                                            pt.length()), PMTMatch);
                    }
                    pos = matchPos + pt.length();
                }
            }
        }
        if (matchHash.size() > 0)
            matches.append(matchHash);
        matchHash.clear();
    }
    if (es)  emit dataMatchingEnd();
    return matches;
}

