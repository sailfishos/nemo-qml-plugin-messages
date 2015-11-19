TARGET = nemomessages-internal
PLUGIN_IMPORT_PATH = org/nemomobile/messages/internal
TEMPLATE = lib
CONFIG += qt plugin hide_symbols

CONFIG += link_pkgconfig

PKGCONFIG += TelepathyQt5 commhistory-qt5 qtcontacts-sqlite-qt5-extensions
QT += qml contacts
target.path = $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH

SOURCES += plugin.cpp \
    src/accountsmodel.cpp \
    src/conversationchannel.cpp \
    src/channelmanager.cpp \
    src/smscharactercounter.cpp \
    src/declarativeaccount.cpp

HEADERS += src/accountsmodel.h \
    src/conversationchannel.h \
    src/channelmanager.h \
    src/smscharactercounter.h \
    src/declarativeaccount.h

INSTALLS += target

qmldir.files += $$PWD/qmldir
qmldir.path +=  $$target.path
INSTALLS += qmldir
