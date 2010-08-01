TEMPLATE = lib
VERSION = 0.9.11
TARGET = siilihai

isEmpty(PREFIX) {
  PREFIX = /usr
}
BINDIR = $$PREFIX/bin
LIBDIR = $$PREFIX/lib
DATADIR = $$PREFIX/share
INCLUDEDIR = $$PREFIX/include/siilihai

CONFIG += create_prl
CONFIG += debug
QMAKE_CXXFLAGS += -g
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
    usersettings.h
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
    usersettings.cpp
target.path = $$LIBDIR
INSTALLS += target
headers.path = $$INCLUDEDIR
headers.files = $$HEADERS
INSTALLS += headers
