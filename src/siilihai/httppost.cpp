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


HttpPost::HttpPost(QNetworkRequest &req, QVariant userVariant, QString clientKey) : QUrlQuery() {
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                   QString("application/x-www-form-urlencoded"));
    if(!userVariant.isNull())
        req.setAttribute(QNetworkRequest::User, userVariant);
    if (!clientKey.isNull()) {
        addQueryItem("client_key", clientKey);
    }
}

HttpPost::~HttpPost() { }

QByteArray HttpPost::postData()
{
    return query(QUrl::FullyEncoded).toUtf8();
}
