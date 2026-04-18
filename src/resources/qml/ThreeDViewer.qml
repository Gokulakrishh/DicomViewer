import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D

Rectangle {
    id: root
    color: "#10151c"
    property real yawAngle: 0
    property real pitchAngle: -25
    property real cameraDistance: Math.max(300, meshGeometry.boundingRadius * 3.0)
    property point lastDragPoint: Qt.point(0, 0)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: "#2f6ea4"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 18

                Label {
                    text: "3D"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 16
                }

                Label {
                    text: viewerController.profileName.length > 0 ? "Profile: " + viewerController.profileName : "Profile: -"
                    color: "white"
                }

                ComboBox {
                    id: profileComboBox
                    model: ["Auto", "Bone", "Lung"]
                    currentIndex: 0
                    Layout.preferredWidth: 110
                    onActivated: function(index) {
                        threeDWindow.setProfileMode(index)
                    }
                }

                Label {
                    text: viewerController.anatomyLabel.length > 0 ? "Anatomy: " + viewerController.anatomyLabel : "Anatomy: -"
                    color: "white"
                }

                Rectangle {
                    width: 18
                    height: 18
                    radius: 4
                    color: viewerController.surfaceColor
                    border.width: 1
                    border.color: "#dfe6ee"
                }

                Label {
                    text: "Vertices: " + viewerController.vertexCount
                    color: "white"
                }

                Label {
                    text: "Triangles: " + viewerController.triangleCount
                    color: "white"
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Drag to rotate, wheel to zoom"
                    color: "#dbe8f5"
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#10151c"

            View3D {
                id: view3d
                anchors.fill: parent

                environment: SceneEnvironment {
                    backgroundMode: SceneEnvironment.Color
                    clearColor: "#10151c"
                }

                PerspectiveCamera {
                    id: camera
                    position: Qt.vector3d(0, 0, root.cameraDistance)
                    clipNear: 1
                    clipFar: 100000
                }

                DirectionalLight {
                    eulerRotation.x: -35
                    eulerRotation.y: 35
                    brightness: 1.2
                }

                DirectionalLight {
                    eulerRotation.x: 55
                    eulerRotation.y: -20
                    brightness: 0.6
                }

                DirectionalLight {
                    eulerRotation.x: 0
                    eulerRotation.y: 180
                    brightness: 0.45
                }

                DirectionalLight {
                    eulerRotation.x: -10
                    eulerRotation.y: -90
                    brightness: 0.35
                }

                Node {
                    id: meshPivot
                    eulerRotation.x: root.pitchAngle
                    eulerRotation.y: root.yawAngle

                    Model {
                        visible: viewerController.meshAvailable
                        geometry: viewerController.meshAvailable ? meshGeometry : null
                        position: Qt.vector3d(
                                      -meshGeometry.meshCenter.x,
                                      -meshGeometry.meshCenter.y,
                                      -meshGeometry.meshCenter.z)
                        materials: [
                            DefaultMaterial {
                                diffuseColor: viewerController.surfaceColor
                                specularAmount: 0.12
                                lighting: DefaultMaterial.FragmentLighting
                                cullMode: Material.NoCulling
                            }
                        ]
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onPressed: function(mouse) {
                    root.lastDragPoint = Qt.point(mouse.x, mouse.y)
                }
                onPositionChanged: function(mouse) {
                    if (!(mouse.buttons & Qt.LeftButton))
                        return

                    const deltaX = mouse.x - root.lastDragPoint.x
                    const deltaY = mouse.y - root.lastDragPoint.y
                    root.yawAngle += deltaX * 0.5
                    root.pitchAngle = Math.max(-89, Math.min(89, root.pitchAngle + deltaY * 0.5))
                    root.lastDragPoint = Qt.point(mouse.x, mouse.y)
                }
                onWheel: function(wheel) {
                    const zoomFactor = wheel.angleDelta.y > 0 ? 0.9 : 1.1
                    root.cameraDistance = Math.max(80, Math.min(20000, root.cameraDistance * zoomFactor))
                }
            }

            Rectangle {
                anchors.centerIn: parent
                width: 360
                visible: viewerController.isBusy || viewerController.errorText.length > 0 || !viewerController.meshAvailable
                radius: 10
                color: "#dbe6f1"
                border.color: "#8aa8c6"
                border.width: 1
                opacity: 0.96

                Column {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 8

                    BusyIndicator {
                        visible: viewerController.isBusy
                        running: viewerController.isBusy
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Label {
                        width: parent.width
                        text: viewerController.isBusy
                              ? "Building 3D mesh in background..."
                              : (viewerController.errorText.length > 0
                                 ? "3D build failed"
                                 : "No 3D mesh available")
                        font.bold: true
                        wrapMode: Text.WordWrap
                        color: "#182431"
                    }

                    Label {
                        width: parent.width
                        text: viewerController.errorText.length > 0
                              ? viewerController.errorText
                              : "Open a multi-slice CT series and build a 3D view."
                        wrapMode: Text.WordWrap
                        color: "#263747"
                    }
                }
            }
        }
    }
}
