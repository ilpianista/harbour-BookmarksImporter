import QtQuick 2.2
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import Nemo.DBus 2.0
import harbour.bookmarksimporter 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    property string filePath: ""

    BookmarkImporter {
        id: importer
    }

    DBusInterface {
        id: systemdUser
        bus: DBus.SessionBus
        service: 'org.freedesktop.systemd1'
        path: '/org/freedesktop/systemd1'
        iface: 'org.freedesktop.systemd1.Manager'
    }

    function restartBrowser() {
        systemdUser.typedCall('RestartUnit', [
            {
                "type": 's',
                "value": 'booster-browser@sailfish-browser.service'
            },
            {
                "type": 's',
                "value": 'fail'
            }
        ], function (result) {
            console.log("sailfish-browser restarted");
            showStatus(qsTr("Browser restarted"), Theme.primaryColor);
        }, function (error, message) {
            console.log("failed (" + error + ") with:", message);
            showStatus(qsTr("Failed to restart the browser"), Theme.highlightColor);
        });
    }

    function showStatus(text, color) {
        statusLabel.text = text;
        statusLabel.color = color;
        statusLabel.visible = true;
    }

    function clearStatus() {
        statusLabel.visible = false;
        statusLabel.text = "";
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        PullDownMenu {
            MenuItem {
                text: qsTr("Restart browser")
                onClicked: restartBrowser()
            }
        }

        Column {
            id: column
            width: page.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("Bookmarks Importer")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("Import bookmarks from an HTML file exported by your browser (e.g. Firefox):")
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Choose HTML file")
                onClicked: pageStack.push(filePickerPage)
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeSmall
                visible: filePath !== ""
                text: qsTr("File: %1").arg(filePath)
            }

            Label {
                id: statusLabel
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeSmall
                visible: false
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                text: qsTr("NOTE: the browser must be restarted for the changes to take effect. Use the pulley menu.")
            }
        }

        VerticalScrollDecorator {}
    }

    Component {
        id: filePickerPage
        FilePickerPage {
            title: qsTr("Choose bookmark file")
            nameFilters: ['*.html', '*.htm']
            onSelectedContentPropertiesChanged: {
                page.filePath = selectedContentProperties.filePath;
                clearStatus();
                var res = importer.parseFile(page.filePath);
                if (res.error !== undefined) {
                    pageStack.pop();
                    showStatus(res.error, Theme.highlightColor);
                } else {
                    pageStack.push(selectPage, {
                        "bookmarkModel": res,
                        "importer": importer,
                        "homePage": page
                    });
                }
            }
        }
    }

    Component {
        id: selectPage
        SelectPage {}
    }
}
