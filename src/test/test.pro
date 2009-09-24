TEMPLATE = app
TARGET = libsiilihaitests
QT += core \
    sql \
    xml \
    network \
    webkit \
    testlib
QT -= gui

HEADERS += libsiilihaitests.h \
	forumdatabase.h \
    parserdatabase.h \
    siilihaiprotocol.h \
    parserengine.h \
    forumparser.h \
    patternmatcher.h
SOURCES += main.cpp \
	libsiilihaitests.cpp
#    forumdatabase.cpp \
#    parserdatabase.cpp \
#    siilihaiprotocol.cpp \
#    parserengine.cpp \
#    forumparser.cpp \
#    patternmatcher.cpp \

INCLUDEPATH += ..
DEPENDPATH += ..
LIBS += -L.. -lsiilihai
FORMS += 
RESOURCES += 
