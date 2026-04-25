import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs

Rectangle {
    height: 44
    color: "#1e1e1e"

    FolderDialog {
        id: folderDialog
        title: "Select MDM Folder"
        onAccepted: appController.loadFolder(selectedFolder)
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 6

        Button {
            text: "Open Folder"
            onClicked: folderDialog.open()

            contentItem: Text {
                text: parent.text
                color: "#ddd"
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                color: parent.pressed ? "#3a3a3a" : (parent.hovered ? "#2e2e2e" : "#262626")
                radius: 5
                border.color: "#444"
                border.width: 1
            }
        }

        Button {
            id: muteBtn
            text: appController.muted ? "Unmute (M)" : "Mute (M)"
            enabled: appController.hasContent
            onClicked: appController.toggleMute()

            contentItem: Text {
                text: parent.text
                color: parent.enabled ? "#ddd" : "#555"
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                color: parent.pressed ? "#3a3a3a" : (parent.hovered ? "#2e2e2e" : "#262626")
                radius: 5
                border.color: "#444"
                border.width: 1
            }
        }

        Item { Layout.fillWidth: true }

        Button {
            text: "Enter VR"
            enabled: false
            opacity: 0.4

            ToolTip.visible: hovered
            ToolTip.text: "VR is not yet supported on macOS"
            ToolTip.delay: 600

            contentItem: Text {
                text: parent.text
                color: "#555"
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                color: "#262626"
                radius: 5
                border.color: "#333"
                border.width: 1
            }
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: "#333"
    }
}
