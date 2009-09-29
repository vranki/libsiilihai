/*
 * patternmatcher.cpp
 *
 *  Created on: Sep 18, 2009
 *      Author: vranki
 */

#include "patternmatcher.h"

PatternMatcher::PatternMatcher(QObject *parent) :
	QObject(parent) {
	// TODO Auto-generated constructor stub
	patternSet = false;
}

PatternMatcher::~PatternMatcher() {
	// TODO Auto-generated destructor stub
}

void PatternMatcher::setPattern(QString &pattern) {
	patternTokens = tokenizePattern(pattern);
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
	//	qDebug() << "Matching pattern " << pattern << " to \n" << html;
	QList<QHash<QString, QString> > matches;
	QHash<QString, QString> matchHash;

	int pos = 0;
	int htmllength = html.length();
	while (pos < htmllength) {
		for (int n = 0; n < patternTokens.length() && pos < htmllength; n++) {
			QString pt = patternTokens[n];
			if (isTag(pt)) {
				if (n == patternTokens.length()) {
					qDebug() << "Panic!! not enough tokens!!";
				}
				QString nextToken = patternTokens[n + 1];
				//				qDebug() << "Looking for token " << pt << " ending at "
				//						<< nextToken << " at " << html.right(htmllength - pos);
				int matchPos = html.indexOf(nextToken, pos);
				if (matchPos < 0) {
					pos = htmllength;
				} else {
					if (pt[1] != 'i') {
						QString match = html.mid(pos, matchPos - pos);

						if (isNumberTag(pt)) {
							match = numberize(match);
						}
						//						qDebug() << "tag " << pt << ":" << match;
						pt = pt.toLower();
						matchHash[pt] = match;
					}
					pos = matchPos;
				}
			} else {
				//				qDebug() << "Looking for text " << pt << " at " << html.right(
				//						htmllength - pos);
				int matchPos = html.indexOf(pt, pos);
				if (matchPos < 0) {
					pos = htmllength;
				} else {
					pos = matchPos + pt.length();
				}
			}
		}
		if (matchHash.size() > 0)
			matches.append(matchHash);
		matchHash.clear();
	}
	return matches;
}
