TEMPLATE = lib
VERSION = 1.3.0
TARGET = siilihai

target.path = $$[QT_INSTALL_LIBS]
INSTALLS += target

CONFIG += qt create_prl

unix {
    CONFIG += debug
    CONFIG -= release
}

QT += core xml network

QT -= gui

debug {
    message(Debug build - enabling sanity checks)
    DEFINES += SANITY_CHECKS
}

release {
    message(Release build - no extra crap)
    DEFINES -= SANITY_CHECKS
}

headers.files += \
    siilihai/syncmaster.h \
    siilihai/forumrequest.h \
    siilihai/httppost.h \
    siilihai/forumsession.h \
    siilihai/siilihaiprotocol.h \
    siilihai/patternmatcher.h \
    siilihai/usersettings.h \
    siilihai/xmlserialization.h \
    siilihai/clientlogic.h \
    siilihai/messageformatting.h \
    siilihai/credentialsrequest.h
headers.path = $$[QT_INSTALL_PREFIX]/include/siilihai

parser.files += siilihai/parser/parserreport.h \
    siilihai/parser/parserdatabase.h \
    siilihai/parser/parserengine.h \
    siilihai/parser/forumparser.h \
    siilihai/parser/parsermanager.h

parser.path += $$headers.path/parser

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

HEADERS += $$parser.files $$forumdata.files $$forumdatabase.files $$headers.files


SOURCES += siilihai/syncmaster.cpp \
    siilihai/parser/parserreport.cpp \
    siilihai/forumrequest.cpp \
    siilihai/forumdata/forummessage.cpp \
    siilihai/forumdata/forumthread.cpp \
    siilihai/forumdata/forumgroup.cpp \
    siilihai/httppost.cpp \
    siilihai/forumsession.cpp \
    siilihai/forumdata/forumsubscription.cpp \
    siilihai/forumdatabase/forumdatabase.cpp \
    siilihai/parser/parserdatabase.cpp \
    siilihai/siilihaiprotocol.cpp \
    siilihai/parser/parserengine.cpp \
    siilihai/parser/forumparser.cpp \
    siilihai/patternmatcher.cpp \
    siilihai/usersettings.cpp \
    siilihai/forumdata/forumdataitem.cpp \
    siilihai/forumdata/updateableitem.cpp \
    siilihai/forumdatabase/forumdatabasexml.cpp \
    siilihai/xmlserialization.cpp \
    siilihai/parser/parsermanager.cpp \
    siilihai/clientlogic.cpp \
    siilihai/messageformatting.cpp \
    siilihai/credentialsrequest.cpp

INSTALLS += headers parser forumdata forumdatabase
