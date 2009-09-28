/*
 * httppost.cpp
 *
 *  Created on: Sep 28, 2009
 *      Author: vranki
 */

#include "httppost.h"

HttpPost::HttpPost() {
	// TODO Auto-generated constructor stub

}

HttpPost::~HttpPost() {
	// TODO Auto-generated destructor stub
}


QByteArray HttpPost::setPostParameters(QNetworkRequest *req, const
		QHash<QString, QString> &params) {
	req->setHeader(QNetworkRequest::ContentTypeHeader, QString(
			"application/x-www-form-urlencoded"));
	QUrl paramsUrl("");
	QHashIterator<QString, QString> i(params);
	while (i.hasNext()) {
		i.next();
		QString val = i.value();
		paramsUrl.addQueryItem(i.key(), val.replace(' ', '+'));
	}
	return paramsUrl.encodedQuery();
}
