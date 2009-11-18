TEMPLATE = app
TARGET = libsiilihaitests
QT += core \
    sql \
    xml \
    network \
    testlib
QT -= gui

HEADERS += libsiilihaitests.h
SOURCES += main.cpp \
	libsiilihaitests.cpp

INCLUDEPATH += ..
DEPENDPATH += ..
LIBS += -L.. -lsiilihai
FORMS += 
RESOURCES += 
