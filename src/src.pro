TEMPLATE = lib
VERSION = 1.1.0
TARGET = siilihai

isEmpty(PREFIX) {
  PREFIX = /usr
}
#BINDIR = $$PREFIX/bin
#DATADIR = $$PREFIX/share
INCLUDEDIR = $$PREFIX/include/siilihai
LIBDIR = $$PREFIX/lib

#!contains(QMAKE_HOST.arch, x86_64) {
#   LIBDIR = $$PREFIX/lib
#} else {
#   LIBDIR = $$PREFIX/lib64
#}

CONFIG += qt create_prl

QT += core xml network

QT -= gui

debug {
    message(Debug build - enabling sanity checks)
    DEFINES += SANITY_CHECKS
}

release {
    message(Release build - no extra crap)
    DEFINES += SANITY_CHECKS
}

HEADERS += siilihai/syncmaster.h \
    siilihai/parserreport.h \
    siilihai/forumrequest.h \
    siilihai/forummessage.h \
    siilihai/forumthread.h \
    siilihai/forumgroup.h \
    siilihai/httppost.h \
    siilihai/forumsession.h \
    siilihai/forumsubscription.h \
    siilihai/forumdatabase.h \
    siilihai/parserdatabase.h \
    siilihai/siilihaiprotocol.h \
    siilihai/parserengine.h \
    siilihai/forumparser.h \
    siilihai/patternmatcher.h \
    siilihai/usersettings.h \
    siilihai/forumdataitem.h \
    siilihai/updateableitem.h \
    siilihai/forumdatabasexml.h \
    siilihai/xmlserialization.h \
    siilihai/parsermanager.h \
    siilihai/clientlogic.h \
    siilihai/messageformatting.h \
    siilihai/credentialsrequest.h

SOURCES += siilihai/syncmaster.cpp \
    siilihai/parserreport.cpp \
    siilihai/forumrequest.cpp \
    siilihai/forummessage.cpp \
    siilihai/forumthread.cpp \
    siilihai/forumgroup.cpp \
    siilihai/httppost.cpp \
    siilihai/forumsession.cpp \
    siilihai/forumsubscription.cpp \
    siilihai/forumdatabase.cpp \
    siilihai/parserdatabase.cpp \
    siilihai/siilihaiprotocol.cpp \
    siilihai/parserengine.cpp \
    siilihai/forumparser.cpp \
    siilihai/patternmatcher.cpp \
    siilihai/usersettings.cpp \
    siilihai/forumdataitem.cpp \
    siilihai/updateableitem.cpp \
    siilihai/forumdatabasexml.cpp \
    siilihai/xmlserialization.cpp \
    siilihai/parsermanager.cpp \
    siilihai/clientlogic.cpp \
    siilihai/messageformatting.cpp \
    siilihai/credentialsrequest.cpp

target.path = $$LIBDIR
INSTALLS += target
headers.path = $$INCLUDEDIR
headers.files = $$HEADERS
INSTALLS += headers
