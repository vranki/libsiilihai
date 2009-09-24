TEMPLATE = lib

VERSION = 0.5

TARGET = siilihai
CONFIG += create_prl

QT += core \
    sql \
    xml \
    network \
    webkit \
    testlib
QT -= gui

HEADERS += forumdatabase.h \
    parserdatabase.h \
    siilihaiprotocol.h \
    parserengine.h \
    forumparser.h \
    patternmatcher.h
SOURCES += forumdatabase.cpp \
    parserdatabase.cpp \
    siilihaiprotocol.cpp \
    parserengine.cpp \
    forumparser.cpp \
    patternmatcher.cpp
BINDIR = $$PREFIX/bin
LIBDIR = $$PREFIX/lib
DATADIR =$$PREFIX/share
INCLUDEDIR =$$PREFIX/include/siilihai

target.path = $$LIBDIR
INSTALLS += target

headers.path = $$INCLUDEDIR
headers.files = $$HEADERS
INSTALLS += headers
