import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    color: "#181818"

    // Top separator line
    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: "#333"
    }

    ListView {
        id: strip
        anchors {
            fill: parent
            topMargin: 1
        }
        orientation: ListView.Horizontal
        spacing: 4
        leftMargin: 8
        rightMargin: 8
        clip: true

        model: appController.thumbnailModel

        // Scroll to keep the active item visible when it changes
        Connections {
            target: appController
            function onCurrentIndexChanged() {
                strip.positionViewAtIndex(appController.currentIndex, ListView.Contain)
            }
        }

        delegate: Item {
            width: 120
            height: strip.height

            required property int index
            required property string imagePath
            required property string displayName
            required property bool hasAudio

            readonly property bool isCurrent: index === appController.currentIndex

            // Thumbnail image via the async provider
            Image {
                id: thumb
                anchors {
                    top: parent.top
                    topMargin: 8
                    horizontalCenter: parent.horizontalCenter
                }
                width: 110
                height: 95
                source: "image://mdmthumbnail/" + encodeURIComponent(imagePath)
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                smooth: true

                // Loading placeholder
                Rectangle {
                    anchors.fill: parent
                    color: "#222"
                    visible: parent.status !== Image.Ready
                    BusyIndicator {
                        anchors.centerIn: parent
                        running: parent.visible
                        implicitWidth: 24
                        implicitHeight: 24
                    }
                }
            }

            // Display name below the thumbnail
            Text {
                anchors {
                    top: thumb.bottom
                    topMargin: 4
                    left: parent.left
                    right: parent.right
                }
                text: displayName
                color: isCurrent ? "#fff" : "#888"
                font.pixelSize: 9
                elide: Text.ElideMiddle
                horizontalAlignment: Text.AlignHCenter
            }

            // Audio indicator dot
            Rectangle {
                anchors {
                    right: thumb.right
                    top: thumb.top
                    margins: 3
                }
                width: 8
                height: 8
                radius: 4
                color: "#4fc3f7"
                visible: hasAudio
            }

            // Selection highlight
            Rectangle {
                anchors.fill: thumb
                color: "transparent"
                border.color: isCurrent ? "#4fc3f7" : "transparent"
                border.width: 2
                radius: 2
            }

            // Click handler
            MouseArea {
                anchors.fill: parent
                onClicked: appController.selectMdm(index)
                cursorShape: Qt.PointingHandCursor
            }
        }
    }
}
