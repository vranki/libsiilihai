/*
 * parserengine.cpp
 *
 *  Created on: Sep 18, 2009
 *      Author: vranki
 */

#include "parserengine.h"

ParserEngine::ParserEngine(QObject *parent) : QObject(parent) {
	// TODO Auto-generated constructor stub

}

ParserEngine::~ParserEngine() {
	// TODO Auto-generated destructor stub
}

void ParserEngine::setParser(ForumParser fp) {
	parser = fp;
}
