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
#include <QObject>
#include <QString>
#include <QDebug>
#include <QString>
#include <QList>

#ifndef PATTERNMATCHER_H_
#define PATTERNMATCHER_H_

enum PatternMatchType { PMTNoMatch, PMTMatch, PMTIgnored, PMTTag, PMTWhitespace };
/**
  * Searches the HTML from web page for matches using given pattern.
  * Can also emit signals during search (used by parser maker for color coding)
  *
  * @see ForumParser
  */
class PatternMatcher : public QObject {
    Q_OBJECT
public:

    PatternMatcher(QObject *parent=0, bool emitStatus=false);
    virtual ~PatternMatcher();
    QList <QHash<QString, QString> > findMatches(QString &html);
    void setPattern(QString &pattern);
    bool isPatternSet();
signals:
    void dataMatchingStart(QString &html);
    void dataMatchingEnd();
    void dataMatched(int pos, QString data, PatternMatchType type);
private:
    QList<QString> tokenizePattern(QString pattern);
    bool isTag(QString &tag);
    bool isNumberTag(QString &tag);
    QString numberize(QString &txt);
    QList<QString> patternTokens;
    bool patternSet, es;
};

#endif /* PATTERNMATCHER_H_ */
