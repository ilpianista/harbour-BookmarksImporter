import QtQuick 2.2
import Sailfish.Silica 1.0
import harbour.bookmarksimporter 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    property var bookmarkModel: []
    property var importer
    property var homePage

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: page.bookmarkModel
        delegate: nodeDelegate

        header: Column {
            width: listView.width

            PageHeader {
                title: qsTr("Import bookmarks")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("Uncheck the bookmarks or folders you want to exclude:")
            }
        }

        footer: Column {
            width: listView.width
            spacing: Theme.paddingLarge

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Import")

                onClicked: {
                    var res = importer.importSelected();
                    pageStack.pop(homePage);
                    if (res.error !== undefined) {
                        homePage.showStatus(res.error, Theme.highlightColor);
                    } else {
                        homePage.showStatus(qsTr("Success: imported %1, skipped %2").arg(res.imported).arg(res.skipped), Theme.primaryColor);
                    }
                }
            }
        }

        VerticalScrollDecorator {}
    }

    Component {
        id: nodeDelegate

        Column {
            width: listView.width

            BackgroundItem {
                id: row
                width: listView.width
                contentHeight: Theme.itemSizeSmall

                onClicked: {
                    if (modelData.isFolder) {
                        modelData.expanded = !modelData.expanded;
                    }
                }

                Row {
                    id: contentRow
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.horizontalPageMargin + modelData.depth * Theme.paddingLarge
                    anchors.right: parent.right
                    anchors.rightMargin: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Theme.paddingMedium

                    Switch {
                        id: checkSwitch
                        anchors.verticalCenter: parent.verticalCenter
                        automaticCheck: false
                        checked: modelData.checkState !== BookmarkNode.Unchecked
                        onClicked: {
                            modelData.checkState = modelData.checkState !== BookmarkNode.Unchecked ? BookmarkNode.Unchecked : BookmarkNode.Checked;
                        }
                    }

                    Label {
                        id: titleLabel
                        width: contentRow.width - checkSwitch.width - chevron.width - contentRow.spacing
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.title
                        color: Theme.primaryColor
                        font.pixelSize: Theme.fontSizeSmall
                        font.weight: modelData.isFolder ? Font.Bold : Font.Normal
                        truncationMode: TruncationMode.Fade
                    }

                    Image {
                        id: chevron
                        visible: modelData.isFolder
                        width: modelData.isFolder ? Theme.iconSizeSmall : 0
                        height: Theme.iconSizeSmall
                        anchors.verticalCenter: parent.verticalCenter
                        source: "image://theme/icon-m-right"
                        rotation: modelData.expanded ? 90 : 0
                        Behavior on rotation {
                            NumberAnimation {
                                duration: 150
                            }
                        }
                    }
                }
            }

            Repeater {
                width: listView.width
                model: modelData.isFolder && modelData.expanded ? modelData.childNodes : 0
                delegate: nodeDelegate
            }
        }
    }
}
