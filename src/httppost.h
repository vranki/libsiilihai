/*
 * httppost.h
 *
 *  Created on: Sep 28, 2009
 *      Author: vranki
 */

#ifndef HTTPPOST_H_
#define HTTPPOST_H_
#include <QByteArray>
#include <QHash>
#include <QString>
#include <QDebug>
#include <QNetworkRequest>


class HttpPost {
public:
	HttpPost();
	virtual ~HttpPost();
	static QByteArray setPostParameters(QNetworkRequest *req, const QHash<QString, QString> &params);
private:
	static void encodeParam(QString &p);
};

#endif /* HTTPPOST_H_ */
