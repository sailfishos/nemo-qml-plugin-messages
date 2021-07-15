TARGET = nemomessages-internal
PLUGIN_IMPORT_PATH = org/nemomobile/messages/internal
TEMPLATE = lib
CONFIG += qt plugin hide_symbols

CONFIG += link_pkgconfig
PKGCONFIG += TelepathyQt5 commhistory-qt5 qtcontacts-sqlite-qt5-extensions

NEMO {
    DEFINES += FOR_NEMO
}

QT = \
    core \
    contacts \
    dbus \
    qml

target.path = $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH

SOURCES += plugin.cpp \
    accountsmodel.cpp \
    conversationchannel.cpp \
    channelmanager.cpp \
    smscharactercounter.cpp \
    mmsmessageprogress.cpp \
    declarativeaccount.cpp \
    smssender.cpp

HEADERS += accountsmodel.h \
    conversationchannel.h \
    channelmanager.h \
    smscharactercounter.h \
    mmsmessageprogress.h \
    declarativeaccount.h \
    smssender.h

OTHER_FILES += mmstransfer.xml \
  mmstransferlist.xml

DBUS_INTERFACES += mmstransfer
mmstransfer.files = mmstransfer.xml
mmstransfer.header_flags = -N -c MmsMessageTransferInterface
mmstransfer.source_flags = -N -c MmsMessageTransferInterface

DBUS_INTERFACES += mmstransferlist
mmstransferlist.files = mmstransferlist.xml
mmstransferlist.header_flags = -N -c MmsMessageTransferListInterface
mmstransferlist.source_flags = -N -c MmsMessageTransferListInterface

INSTALLS += target

qmldir.files += qmldir plugins.qmltypes
qmldir.path +=  $$target.path
INSTALLS += qmldir

qmltypes.commands = qmlplugindump -nonrelocatable org.nemomobile.messages.internal 1.0 > $$PWD/plugins.qmltypes
QMAKE_EXTRA_TARGETS += qmltypes
