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

QT += core xml network
QT -= gui
CONFIG += c++14

debug {
#    message(Debug build - enabling sanity checks)
#    DEFINES += SANITY_CHECKS
}

release {
    message(Release build - no extra crap)
    DEFINES -= SANITY_CHECKS
}

include(libsiilihai.pri)

headers.path = $$[QT_INSTALL_PREFIX]/include/siilihai

parser.path += $$headers.path/parser

tapatalk.path += $$headers.path/tapatalk

discourse.path += $$headers.path/discourse

forumdata.path += $$headers.path/forumdata

forumdatabase.path = $$headers.path/forumdatabase

INSTALLS += headers tapatalk parser discourse forumdata forumdatabase

DISTFILES += libsiilihai.pri
