TEMPLATE = app
TARGET = libsiilihaitests
QT += core \
    sql \
    xml \
    network \
    testlib
QT -= gui
HEADERS += ../../debian/libsiilihai/opt/maemo/usr/include/siilihai/forumsession.h \
    ../../debian/libsiilihai/opt/maemo/usr/include/siilihai/siilihaiprotocol.h \
    ../../debian/libsiilihai/usr/include/siilihai/forumdatabase.h \
    ../../debian/libsiilihai/usr/include/siilihai/forumgroup.h \
    ../../debian/libsiilihai/usr/include/siilihai/forummessage.h \
    ../../debian/libsiilihai/usr/include/siilihai/forumparser.h \
    ../../debian/libsiilihai/usr/include/siilihai/forumrequest.h \
    ../../debian/libsiilihai/usr/include/siilihai/forumsubscription.h \
    ../../debian/libsiilihai/usr/include/siilihai/forumthread.h \
    ../../debian/libsiilihai/usr/include/siilihai/httppost.h \
    ../../debian/libsiilihai/usr/include/siilihai/parserdatabase.h \
    ../../debian/libsiilihai/usr/include/siilihai/parserengine.h \
    ../../debian/libsiilihai/usr/include/siilihai/parserreport.h \
    ../../debian/libsiilihai/usr/include/siilihai/patternmatcher.h \
    libsiilihaitests.h
SOURCES += main.cpp \
    libsiilihaitests.cpp
INCLUDEPATH += ..
DEPENDPATH += ..
LIBS += -L.. \
    -lsiilihai
FORMS += 
RESOURCES += 
