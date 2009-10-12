#include <QObject>
#include <QString>
#include <QDebug>
#include <QString>
#include <QList>

#ifndef PATTERNMATCHER_H_
#define PATTERNMATCHER_H_

enum PatternMatchType { PMTNoMatch, PMTMatch, PMTIgnored, PMTTag };

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
