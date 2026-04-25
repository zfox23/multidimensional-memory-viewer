import QtQuick
import QtQuick.Controls.Basic

Item {
    id: root

    // Background is black for the back hemisphere
    Rectangle {
        anchors.fill: parent
        color: "#000"
    }

    // The equirectangular source image, loaded async by Qt Quick.
    // visible:false — it is only used as a texture source for the ShaderEffect.
    Image {
        id: equiImage
        source: appController.hasContent
                ? ("file://" + appController.currentImagePath)
                : ""
        visible: false
        asynchronous: true
        cache: false
        smooth: true

        // Prevent Qt Quick from scaling the image down before it reaches the shader.
        // The shader itself handles all sampling geometry.
        fillMode: Image.Pad
    }

    // Loading indicator while a new MDM image is decoding
    BusyIndicator {
        anchors.centerIn: parent
        running: equiImage.status === Image.Loading
        visible: running
    }

    // The hemisphere ShaderEffect — fills the entire scene area
    ShaderEffect {
        id: shaderEffect
        anchors.fill: parent

        // Bound to the Image above; Qt Quick provides it as a sampler2D named "equiTex"
        property var equiTex: equiImage

        // Camera orientation driven by AppController (also written back on drag)
        property real yaw:         appController.yaw
        property real pitch:       appController.pitch
        property real fov:         1.5707963   // 90° vertical FOV (π/2 rad)
        property real aspectRatio: width > 0 && height > 0 ? width / height : 1.777

        vertexShader:   "qrc:/shaders/hemisphere.vert.qsb"
        fragmentShader: "qrc:/shaders/hemisphere.frag.qsb"

        // Keep aspect ratio in sync if the window resizes
        onWidthChanged:  aspectRatio = width / height
        onHeightChanged: aspectRatio = width / height
    }

    // Mouse/trackpad drag to pan
    MouseArea {
        id: dragArea
        anchors.fill: parent
        hoverEnabled: false

        property real lastX: 0
        property real lastY: 0

        // Sensitivity: radians of rotation per pixel
        readonly property real kSens: 0.004

        onPressed: (mouse) => {
            lastX = mouse.x
            lastY = mouse.y
        }

        onPositionChanged: (mouse) => {
            if (!pressed) return
            const dx = mouse.x - lastX
            const dy = mouse.y - lastY
            lastX = mouse.x
            lastY = mouse.y

            const newYaw   = appController.yaw   - dx * kSens
            const newPitch = appController.pitch  - dy * kSens
            appController.yaw   = newYaw
            appController.pitch = Math.max(-1.48, Math.min(1.48, newPitch))
        }
    }

    // "No content" placeholder shown before a folder is opened
    Column {
        anchors.centerIn: parent
        spacing: 16
        visible: !appController.hasContent

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "No MDMs loaded"
            color: "#888"
            font.pixelSize: 22
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Use File → Open Folder to select an MDM folder"
            color: "#555"
            font.pixelSize: 14
        }
    }
}
