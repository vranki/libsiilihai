TEMPLATE = subdirs
SUBDIRS = src #src/test
OTHER_FILES = debian/control debian/rules debian/changelog debian/copyright\
    debian/libsiilihai-dbg.install debian/libsiilihai1.install debian/libsiilihai1-dev.install\
    qtc_packaging/debian_harmattan/rules \
    qtc_packaging/debian_harmattan/README \
    qtc_packaging/debian_harmattan/manifest.aegis \
    qtc_packaging/debian_harmattan/copyright \
    qtc_packaging/debian_harmattan/control \
    qtc_packaging/debian_harmattan/compat \
    qtc_packaging/debian_harmattan/changelog
