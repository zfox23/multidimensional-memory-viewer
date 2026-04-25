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

        Connections {
            target: appController
            function onCurrentIndexChanged() {
                strip.positionViewAtIndex(appController.currentIndex, ListView.Contain)
            }
        }

        delegate: Item {
            id: delegateRoot
            width: 120
            height: strip.height

            required property int index
            required property string imagePath
            required property string displayName
            required property bool hasAudio
            required property var waveform

            readonly property bool isCurrent: index === appController.currentIndex

            // ── Thumbnail image ───────────────────────────────────────────────
            Image {
                id: thumb
                anchors {
                    top: parent.top
                    topMargin: 8
                    horizontalCenter: parent.horizontalCenter
                }
                width: 110
                height: 80
                // Provider returns an exact left-eye square; show it in full
                source: "image://mdmthumbnail/" + encodeURIComponent(imagePath)
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                smooth: true

                Rectangle {
                    anchors.fill: parent
                    color: "#222"
                    visible: parent.status !== Image.Ready
                    BusyIndicator {
                        anchors.centerIn: parent
                        running: parent.visible
                        implicitWidth: 20
                        implicitHeight: 20
                    }
                }
            }

            // ── Display name ──────────────────────────────────────────────────
            Text {
                id: nameLabel
                anchors {
                    top: thumb.bottom
                    topMargin: 3
                    left: parent.left
                    right: parent.right
                }
                text: displayName
                color: isCurrent ? "#fff" : "#888"
                font.pixelSize: 9
                elide: Text.ElideMiddle
                horizontalAlignment: Text.AlignHCenter
            }

            // ── Waveform ──────────────────────────────────────────────────────
            Item {
                id: waveArea
                anchors {
                    top: nameLabel.bottom
                    topMargin: 4
                    horizontalCenter: parent.horizontalCenter
                }
                width: 110
                height: 28

                Canvas {
                    id: waveCanvas
                    anchors.fill: parent

                    onPaint: {
                        const ctx = getContext("2d");
                        ctx.clearRect(0, 0, width, height);

                        const data = delegateRoot.waveform;
                        const centerY = height / 2;
                        const active = delegateRoot.isCurrent;

                        if (!data || data.length === 0) {
                            // Placeholder dashes while waveform loads
                            ctx.fillStyle = "#2a2a2a";
                            const dashes = 40;
                            const dw = width / dashes;
                            for (let i = 0; i < dashes; i++)
                                ctx.fillRect(i * dw, centerY - 1.5, Math.max(1, dw - 1), 3);
                            return;
                        }

                        const n = data.length;
                        const barW = width / n;
                        ctx.fillStyle = active ? "#4fc3f7" : "#2a5a70";

                        for (let i = 0; i < n; i++) {
                            const h = Math.max(1.5, data[i] * height * 0.88);
                            ctx.fillRect(
                                i * barW,
                                centerY - h * 0.5,
                                Math.max(0.5, barW - 0.5),
                                h
                            );
                        }
                    }

                    // Repaint when waveform data arrives
                    Connections {
                        target: delegateRoot
                        function onWaveformChanged() { waveCanvas.requestPaint(); }
                        function onIsCurrentChanged() { waveCanvas.requestPaint(); }
                    }
                    Component.onCompleted: requestPaint()
                }

                // ── Playback progress line ─────────────────────────────────
                Rectangle {
                    visible: delegateRoot.isCurrent && delegateRoot.hasAudio
                    width: 2
                    height: parent.height
                    color: "#ffffff"
                    opacity: 0.75
                    x: appController.audioPosition * parent.width - 1
                    z: 1
                }
            }

            // ── Audio indicator dot ───────────────────────────────────────────
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

            // ── Selection highlight ───────────────────────────────────────────
            Rectangle {
                anchors.fill: thumb
                color: "transparent"
                border.color: isCurrent ? "#4fc3f7" : "transparent"
                border.width: 2
                radius: 2
            }

            // ── Click handler ─────────────────────────────────────────────────
            MouseArea {
                anchors.fill: parent
                onClicked: appController.selectMdm(index)
                cursorShape: Qt.PointingHandCursor
            }
        }
    }
}
