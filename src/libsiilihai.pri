headers.files += \
    $$PWD/siilihai/syncmaster.h \
    $$PWD/siilihai/forumrequest.h \
    $$PWD/siilihai/httppost.h \
    $$PWD/siilihai/siilihaiprotocol.h \
    $$PWD/siilihai/usersettings.h \
    $$PWD/siilihai/xmlserialization.h \
    $$PWD/siilihai/clientlogic.h \
    $$PWD/siilihai/subscriptionmanagement.h \
    $$PWD/siilihai/messageformatting.h \
    $$PWD/siilihai/credentialsrequest.h \
    $$PWD/siilihai/forumprobe.h \
    $$PWD/siilihai/updateengine.h \
    $$PWD/siilihai/siilihaisettings.h

parser.files += $$PWD/siilihai/parser/parserreport.h \
    $$PWD/siilihai/parser/parserdatabase.h \
    $$PWD/siilihai/parser/parserengine.h \
    $$PWD/siilihai/parser/forumparser.h \
    $$PWD/siilihai/parser/forumsubscriptionparsed.h \
    $$PWD/siilihai/parser/parsermanager.h \
    $$PWD/siilihai/parser/patternmatcher.h

discourse.files += $$PWD/siilihai/discourse/forumsubscriptiondiscourse.h \
                   $$PWD/siilihai/discourse/discourseengine.h

tapatalk.files += $$PWD/siilihai/tapatalk/forumsubscriptiontapatalk.h \
                  $$PWD/siilihai/tapatalk/tapatalkengine.h

forumdata.files += $$PWD/siilihai/forumdata/forummessage.h \
    $$PWD/siilihai/forumdata/forumthread.h \
    $$PWD/siilihai/forumdata/forumgroup.h \
    $$PWD/siilihai/forumdata/forumsubscription.h \
    $$PWD/siilihai/forumdata/forumdataitem.h \
    $$PWD/siilihai/forumdata/updateableitem.h \
    $$PWD/siilihai/forumdata/updateerror.h

forumdatabase.files += $$PWD/siilihai/forumdatabase/forumdatabase.h \
                       $$PWD/siilihai/forumdatabase/forumdatabasexml.h

HEADERS += $$tapatalk.files $$parser.files $$discourse.files $$forumdata.files $$forumdatabase.files $$headers.files

SOURCES += $$PWD/siilihai/syncmaster.cpp \
    $$PWD/siilihai/parser/parserreport.cpp \
    $$PWD/siilihai/forumrequest.cpp \
    $$PWD/siilihai/forumdata/forummessage.cpp \
    $$PWD/siilihai/forumdata/forumthread.cpp \
    $$PWD/siilihai/forumdata/forumgroup.cpp \
    $$PWD/siilihai/httppost.cpp \
    $$PWD/siilihai/forumdata/forumsubscription.cpp \
    $$PWD/siilihai/forumdatabase/forumdatabase.cpp \
    $$PWD/siilihai/parser/parserdatabase.cpp \
    $$PWD/siilihai/siilihaiprotocol.cpp \
    $$PWD/siilihai/usersettings.cpp \
    $$PWD/siilihai/forumdata/forumdataitem.cpp \
    $$PWD/siilihai/forumdata/updateableitem.cpp \
    $$PWD/siilihai/forumdatabase/forumdatabasexml.cpp \
    $$PWD/siilihai/xmlserialization.cpp \
    $$PWD/siilihai/clientlogic.cpp \
    $$PWD/siilihai/subscriptionmanagement.cpp \
    $$PWD/siilihai/messageformatting.cpp \
    $$PWD/siilihai/credentialsrequest.cpp \
    $$PWD/siilihai/updateengine.cpp \
    $$PWD/siilihai/forumprobe.cpp \
    $$PWD/siilihai/siilihaisettings.cpp \
    $$PWD/siilihai/parser/forumsubscriptionparsed.cpp \
    $$PWD/siilihai/parser/parsermanager.cpp \
    $$PWD/siilihai/parser/parserengine.cpp \
    $$PWD/siilihai/parser/forumparser.cpp \
    $$PWD/siilihai/parser/patternmatcher.cpp \
    $$PWD/siilihai/tapatalk/forumsubscriptiontapatalk.cpp \
    $$PWD/siilihai/tapatalk/tapatalkengine.cpp \
    $$PWD/siilihai/forumdata/updateerror.cpp \
    $$PWD/siilihai/discourse/discourseengine.cpp \
    $$PWD/siilihai/discourse/forumsubscriptiondiscourse.cpp
