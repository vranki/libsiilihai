#include <QObject>
#include <QString>
#include <QDebug>
#include <QString>
#include <QList>

#ifndef PATTERNMATCHER_H_
#define PATTERNMATCHER_H_

class PatternMatcher : public QObject {
	Q_OBJECT
public:
	PatternMatcher(QObject *parent=0);
	virtual ~PatternMatcher();
	QList<QString> tokenizePattern(QString pattern);
	QList <QHash<QString, QString> > findMatches(QString &html, QString &pattern);
	bool isTag(QString &tag);
	bool isNumberTag(QString &tag);
	QString numberize(QString &txt);
};

#endif /* PATTERNMATCHER_H_ */
