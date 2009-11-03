#ifndef PARSERREPORT_H_
#define PARSERREPORT_H_

#include <QString>

class ParserReport {
public:
	enum ParserReportType {PRTNoType=0, PRTWorking=1, PRTNotWorking=2, PRTComment=3};

	ParserReport();
	virtual ~ParserReport();

	ParserReportType type;
	QString comment;
	int parserid;
};

#endif /* PARSERREPORT_H_ */
