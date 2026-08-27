TARGET = harbour-bookmarksimporter

CONFIG += sailfishapp
PKGCONFIG += sailfishapp

SOURCES += \
    src/main.cpp \
    src/bookmarkimporter.cpp

HEADERS += \
    src/bookmarkimporter.h

DISTFILES += \
    qml/harbour-bookmarksimporter.qml \
    qml/cover/CoverPage.qml \
    qml/pages/MainPage.qml \
    qml/pages/SelectPage.qml \
    rpm/harbour-bookmarksimporter.spec \
    rpm/harbour-bookmarksimporter.changes \
    translations/*.ts \
    harbour-bookmarksimporter.desktop

CONFIG += sailfishapp_i18n

TRANSLATIONS += \
    translations/harbour-bookmarksimporter.ts

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172
