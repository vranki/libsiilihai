TEMPLATE = lib
VERSION = 0.9.20
TARGET = siilihai

DESTDIR = .

isEmpty(PREFIX) {
  PREFIX = /usr
}
BINDIR = $$PREFIX/bin
DATADIR = $$PREFIX/share
INCLUDEDIR = $$PREFIX/include/siilihai

!contains(QMAKE_HOST.arch, x86_64) {
   LIBDIR = $$PREFIX/lib
} else {
   LIBDIR = $$PREFIX/lib64
}

CONFIG += qt create_prl

QT += core \
    sql \
    xml \
    network

QT -= gui

HEADERS += syncmaster.h \
    parserreport.h \
    forumrequest.h \
    forummessage.h \
    forumthread.h \
    forumgroup.h \
    httppost.h \
    forumsession.h \
    forumsubscription.h \
    forumdatabase.h \
    parserdatabase.h \
    siilihaiprotocol.h \
    parserengine.h \
    forumparser.h \
    patternmatcher.h \
    usersettings.h \
    forumdataitem.h

SOURCES += syncmaster.cpp \
    parserreport.cpp \
    forumrequest.cpp \
    forummessage.cpp \
    forumthread.cpp \
    forumgroup.cpp \
    httppost.cpp \
    forumsession.cpp \
    forumsubscription.cpp \
    forumdatabase.cpp \
    parserdatabase.cpp \
    siilihaiprotocol.cpp \
    parserengine.cpp \
    forumparser.cpp \
    patternmatcher.cpp \
    usersettings.cpp \
    forumdataitem.cpp

target.path = $$LIBDIR
INSTALLS += target
headers.path = $$INCLUDEDIR
headers.files = $$HEADERS
INSTALLS += headers
