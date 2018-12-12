include(../package.pri)

SRCDIR = ../../src/
INCLUDEPATH += $$SRCDIR
DEPENDPATH = $$INCLUDEPATH

QT = \
    core \
    testlib
TEMPLATE = app
CONFIG -= app_bundle

target.path = /opt/tests/$${PACKAGENAME}
INSTALLS += target
