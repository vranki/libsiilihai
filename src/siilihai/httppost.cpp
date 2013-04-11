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
	return resultData.toUtf8();
}
