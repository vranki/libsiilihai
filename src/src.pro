TEMPLATE = lib
VERSION = 2.0.0
TARGET = siilihai

target.path = $$[QT_INSTALL_LIBS]
INSTALLS += target

CONFIG += qt create_prl

unix {
    CONFIG += debug
    CONFIG -= release
}

QT += core xml network webkitwidgets

QT -= gui
CONFIG += c++11


debug {
#    message(Debug build - enabling sanity checks)
#    DEFINES += SANITY_CHECKS
}

release {
    message(Release build - no extra crap)
    DEFINES -= SANITY_CHECKS
}

headers.files += \
    siilihai/syncmaster.h \
    siilihai/forumrequest.h \
    siilihai/httppost.h \
    siilihai/siilihaiprotocol.h \
    siilihai/usersettings.h \
    siilihai/xmlserialization.h \
    siilihai/clientlogic.h \
    siilihai/messageformatting.h \
    siilihai/credentialsrequest.h \
    siilihai/forumprobe.h \
    siilihai/updateengine.h \
    siilihai/siilihaisettings.h
headers.path = $$[QT_INSTALL_PREFIX]/include/siilihai

parser.files += siilihai/parser/parserreport.h \
    siilihai/parser/parserdatabase.h \
    siilihai/parser/parserengine.h \
    siilihai/parser/forumparser.h \
    siilihai/parser/forumsubscriptionparsed.h \
    siilihai/parser/parsermanager.h \
    siilihai/parser/patternmatcher.h

parser.path += $$headers.path/parser

tapatalk.files += siilihai/tapatalk/forumsubscriptiontapatalk.h
tapatalk.path += $$headers.path/tapatalk

forumdata.files += siilihai/forumdata/forummessage.h \
    siilihai/forumdata/forumthread.h \
    siilihai/forumdata/forumgroup.h \
    siilihai/forumdata/forumsubscription.h \
    siilihai/forumdata/forumdataitem.h \
    siilihai/forumdata/updateableitem.h
forumdata.path += $$headers.path/forumdata

forumdatabase.files += siilihai/forumdatabase/forumdatabase.h \
    siilihai/forumdatabase/forumdatabasexml.h
forumdatabase.path = $$headers.path/forumdatabase

HEADERS += $$tapatalk.files $$parser.files $$forumdata.files $$forumdatabase.files $$headers.files \
    siilihai/tapatalk/tapatalkengine.h


SOURCES += siilihai/syncmaster.cpp \
    siilihai/parser/parserreport.cpp \
    siilihai/forumrequest.cpp \
    siilihai/forumdata/forummessage.cpp \
    siilihai/forumdata/forumthread.cpp \
    siilihai/forumdata/forumgroup.cpp \
    siilihai/httppost.cpp \
    siilihai/forumdata/forumsubscription.cpp \
    siilihai/forumdatabase/forumdatabase.cpp \
    siilihai/parser/parserdatabase.cpp \
    siilihai/siilihaiprotocol.cpp \
    siilihai/usersettings.cpp \
    siilihai/forumdata/forumdataitem.cpp \
    siilihai/forumdata/updateableitem.cpp \
    siilihai/forumdatabase/forumdatabasexml.cpp \
    siilihai/xmlserialization.cpp \
    siilihai/clientlogic.cpp \
    siilihai/messageformatting.cpp \
    siilihai/credentialsrequest.cpp \
    siilihai/updateengine.cpp \
    siilihai/forumprobe.cpp \
    siilihai/siilihaisettings.cpp \
    siilihai/parser/forumsubscriptionparsed.cpp \
    siilihai/parser/parsermanager.cpp \
    siilihai/parser/parserengine.cpp \
    siilihai/parser/forumparser.cpp \
    siilihai/parser/patternmatcher.cpp \
    siilihai/tapatalk/forumsubscriptiontapatalk.cpp \
    siilihai/tapatalk/tapatalkengine.cpp

INSTALLS += headers tapatalk parser forumdata forumdatabase
