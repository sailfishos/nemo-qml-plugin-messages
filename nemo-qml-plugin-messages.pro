TEMPLATE = subdirs
SUBDIRS = src tests
OTHER_FILES += rpm/nemo-qml-plugin-messages-internal-qt5.spec

tests.depends = src
