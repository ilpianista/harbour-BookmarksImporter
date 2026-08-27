#include "bookmarkimporter.h"

#include <sailfishapp.h>
#include <QGuiApplication>
#include <QtQuick>

int main(int argc, char *argv[])
{
    QGuiApplication *app = SailfishApp::application(argc, argv);

    app->setOrganizationName(QStringLiteral("dev.scarpino"));
    app->setApplicationName(QStringLiteral("harbour-bookmarksimporter"));

    qmlRegisterType<BookmarkImporter>("harbour.bookmarksimporter", 1, 0, "BookmarkImporter");
    qmlRegisterUncreatableType<BookmarkNode>("harbour.bookmarksimporter",
                                             1,
                                             0,
                                             "BookmarkNode",
                                             QStringLiteral(
                                                 "BookmarkNode cannot be created from QML"));

    return SailfishApp::main(argc, argv);
}
