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

#ifndef FORUMREQUEST_H_
#define FORUMREQUEST_H_
#include <QString>

class ForumRequest {
public:
	ForumRequest();
	virtual ~ForumRequest();
	QString forum_url;
	QString date;
	QString user;
	QString comment;
};

#endif /* FORUMREQUEST_H_ */
