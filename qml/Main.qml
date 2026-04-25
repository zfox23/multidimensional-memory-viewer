import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 800
    minimumWidth: 640
    minimumHeight: 480
    title: appController.hasContent
           ? appController.currentDisplayName + " — Multidimensional Memory Viewer"
           : "Multidimensional Memory Viewer"

    color: "#111"

    // Global mute shortcut
    Shortcut {
        sequence: "M"
        onActivated: appController.toggleMute()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        AppToolBar {
            Layout.fillWidth: true
        }

        ViewerScene {
            id: viewerScene
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        ThumbnailStrip {
            Layout.fillWidth: true
            Layout.preferredHeight: 140
            visible: appController.hasContent
        }
    }
}
