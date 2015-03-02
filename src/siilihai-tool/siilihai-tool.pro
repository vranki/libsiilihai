TEMPLATE = app
TARGET = siilihai-tool
QT += core \
    sql \
    xml \
    network
QT -= gui
HEADERS += siilihaitool.h
SOURCES += main.cpp siilihaitool.cpp
INCLUDEPATH += ..
DEPENDPATH += ..
LIBS += -L.. -lsiilihai

target.path = $$[QT_INSTALL_PREFIX]/bin
INSTALLS += target

CONFIG += c++11
