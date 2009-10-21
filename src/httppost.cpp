/*
 * httppost.cpp
 *
 *  Created on: Sep 28, 2009
 *      Author: vranki
 */

#include "httppost.h"

HttpPost::HttpPost() {
}

HttpPost::~HttpPost() {
}

void HttpPost::encodeParam(QString &p) {
	// Tim Berners-Lee, why is this made so difficult?
	QString encoded;

	for(int i=0;i<p.length();i++) {
		if(p[i]==' ') {
			encoded += "+";
		} else {
			encoded += QUrl::toPercentEncoding(QString(p[i]));
		}
	}
	p = encoded;
}

QByteArray HttpPost::setPostParameters(QNetworkRequest *req, const
		QHash<QString, QString> &params) {
	req->setHeader(QNetworkRequest::ContentTypeHeader, QString(
			"application/x-www-form-urlencoded"));
	QString resultData;
	QUrl paramsUrl("");
	QHashIterator<QString, QString> i(params);
	while (i.hasNext()) {
		i.next();
		QString val = i.value();
		encodeParam(val);
		resultData += i.key() + "=" + val;
		if(i.hasNext())
			resultData +="&";
	}
	return resultData.toAscii();
}
