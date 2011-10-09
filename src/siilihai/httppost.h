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

#ifndef HTTPPOST_H_
#define HTTPPOST_H_
#include <QByteArray>
#include <QHash>
#include <QString>
#include <QDebug>
#include <QNetworkRequest>

/**
  * Surprisingly Qt doesn't have functionality for making
  * HTTP POST requests. This implements HTTP POST in a
  * (hopefully) decent way.
  */
class HttpPost {
public:
    HttpPost();
    virtual ~HttpPost();
    static QByteArray setPostParameters(QNetworkRequest *req, const QHash<QString, QString> &params);
private:
    static void encodeParam(QString &p);
};

#endif /* HTTPPOST_H_ */
