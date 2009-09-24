/*
 * parserengine.h
 *
 *  Created on: Sep 18, 2009
 *      Author: vranki
 */

#ifndef PARSERENGINE_H_
#define PARSERENGINE_H_
#include <QObject>
#include "forumparser.h"

class ParserEngine : public QObject {
	Q_OBJECT

public:
	ParserEngine(QObject *parent=0);
	virtual ~ParserEngine();
	void setParser(ForumParser fp);
private:
	ForumParser parser;
};

#endif /* PARSERENGINE_H_ */
