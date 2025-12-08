pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import OpenGeoLab

Window {
    id: root
    visible: true
    width: 1200
    height: 800
    title: "OpenGeoLab - 3D Geometry Renderer"

    Component.onCompleted: {
        ModelImporter.setTargetRenderer(geometryRenderer);
    }

    Connections {
        target: ModelImporter
        function onModelLoaded(filename) {
            statusText.text = "Loaded: " + filename;
            statusText.color = "lightgreen";
        }
        function onModelLoadFailed(error) {
            statusText.text = "Error: " + error;
            statusText.color = "red";
        }
    }

    // File dialog for model import
    FileDialog {
        id: fileDialog
        title: "Import Model"
        nameFilters: ["STEP files (*.stp *.step)", "BREP files (*.brep *.brp)", "All files (*)"]
        onAccepted: {
            statusText.text = "Loading model...";
            statusText.color = "yellow";
            ModelImporter.importModel(selectedFile);
        }
    }

    // ========================================================================
    // Ribbon Toolbar at top
    // ========================================================================
    RibbonToolBar {
        id: ribbonToolBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top

        onOpenFile: fileDialog.open()
        onImportModel: fileDialog.open()
        onSaveFile: console.log("Save file - TODO")
        onExportModel: console.log("Export model - TODO")

        onAddBox: geometryRenderer.geometryType = "cube"
    }

    // Status text overlay
    Text {
        id: statusText
        color: "white"
        font.pixelSize: 14
        font.bold: true
        text: "Ready to import model"
        anchors.top: ribbonToolBar.bottom
        anchors.left: controlPanel.right
        anchors.margins: 15
        z: 100

        // Background for better visibility
        Rectangle {
            anchors.fill: parent
            anchors.margins: -5
            color: Qt.rgba(0, 0, 0, 0.7)
            radius: 5
            z: -1
        }
    }

    // 3D Geometry renderer - fills most of the window, leaving space for control panel and ribbon
    Geometry3D {
        id: geometryRenderer
        anchors.left: controlPanel.right
        anchors.right: parent.right
        anchors.top: ribbonToolBar.bottom
        anchors.bottom: parent.bottom

        // Default: use vertex colors (alpha = 0)
        color: Qt.rgba(0, 0, 0, 0)
    }
    // Left control panel
    Rectangle {
        id: controlPanel
        width: 200
        anchors.left: parent.left
        anchors.top: ribbonToolBar.bottom
        anchors.bottom: parent.bottom
        color: Qt.rgba(0.2, 0.2, 0.2, 0.9)

        ScrollView {
            anchors.fill: parent
            anchors.margins: 15
            clip: true

            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ColumnLayout {
                width: controlPanel.width - 30
                spacing: 20

                // Title
                Text {
                    text: "Color Control"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                // Separator
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 2
                    color: Qt.rgba(1, 1, 1, 0.3)
                }

                // Geometry type selection
                Text {
                    text: "Geometry Type:"
                    color: "white"
                    font.pixelSize: 14
                }

                // Model Import Button
                Button {
                    text: "Import Model"
                    Layout.fillWidth: true
                    onClicked: {
                        fileDialog.open();
                    }
                }

                // Separator
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 2
                    color: Qt.rgba(1, 1, 1, 0.3)
                }
                Button {
                    text: "Cube"
                    Layout.fillWidth: true
                    highlighted: geometryRenderer.geometryType === "cube"
                    onClicked: {
                        geometryRenderer.geometryType = "cube";
                    }
                }

                Button {
                    text: "Cylinder"
                    Layout.fillWidth: true
                    highlighted: geometryRenderer.geometryType === "cylinder"
                    onClicked: {
                        geometryRenderer.geometryType = "cylinder";
                    }
                }

                // Separator
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 2
                    color: Qt.rgba(1, 1, 1, 0.3)
                }

                // Color selection area
                Text {
                    text: "Select Color:"
                    color: "white"
                    font.pixelSize: 14
                }

                Button {
                    text: "Use Vertex Colors"
                    Layout.fillWidth: true
                    highlighted: geometryRenderer.color.a === 0
                    onClicked: {
                        geometryRenderer.color = Qt.rgba(0, 0, 0, 0);
                    }
                }

                Button {
                    text: "Red"
                    Layout.fillWidth: true
                    highlighted: geometryRenderer.color.r > 0.9 && geometryRenderer.color.a > 0
                    onClicked: {
                        geometryRenderer.color = Qt.rgba(1, 0, 0, 1);
                    }
                }

                Button {
                    text: "Green"
                    Layout.fillWidth: true
                    highlighted: geometryRenderer.color.g > 0.9 && geometryRenderer.color.a > 0
                    onClicked: {
                        geometryRenderer.color = Qt.rgba(0, 1, 0, 1);
                    }
                }

                Button {
                    text: "Blue"
                    Layout.fillWidth: true
                    highlighted: geometryRenderer.color.b > 0.9 && geometryRenderer.color.a > 0
                    onClicked: {
                        geometryRenderer.color = Qt.rgba(0, 0, 1, 1);
                    }
                }

                Button {
                    text: "Yellow"
                    Layout.fillWidth: true
                    highlighted: geometryRenderer.color.r > 0.9 && geometryRenderer.color.g > 0.9 && geometryRenderer.color.a > 0
                    onClicked: {
                        geometryRenderer.color = Qt.rgba(1, 1, 0, 1);
                    }
                }

                Button {
                    text: "Cyan"
                    Layout.fillWidth: true
                    highlighted: geometryRenderer.color.g > 0.9 && geometryRenderer.color.b > 0.9 && geometryRenderer.color.a > 0
                    onClicked: {
                        geometryRenderer.color = Qt.rgba(0, 1, 1, 1);
                    }
                }

                Button {
                    text: "Magenta"
                    Layout.fillWidth: true
                    highlighted: geometryRenderer.color.r > 0.9 && geometryRenderer.color.b > 0.9 && geometryRenderer.color.a > 0
                    onClicked: {
                        geometryRenderer.color = Qt.rgba(1, 0, 1, 1);
                    }
                }

                Button {
                    text: "White"
                    Layout.fillWidth: true
                    highlighted: geometryRenderer.color.r > 0.9 && geometryRenderer.color.g > 0.9 && geometryRenderer.color.b > 0.9 && geometryRenderer.color.a > 0
                    onClicked: {
                        geometryRenderer.color = Qt.rgba(1, 1, 1, 1);
                    }
                }

                // Bottom spacing
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 20
                }
            }
        }
    }

    // Information overlay
    Rectangle {
        color: Qt.rgba(1, 1, 1, 0.7)
        radius: 10
        border.width: 1
        border.color: "white"
        anchors.fill: label
        anchors.margins: -10
    }

    Text {
        id: label
        color: "black"
        wrapMode: Text.WordWrap
        text: qsTr("3D Geometry Renderer - Use Ribbon toolbar for file operations and geometry tools.\nDrag with left mouse button to rotate, Shift+drag to pan, scroll wheel to zoom.")
        anchors.right: parent.right
        anchors.left: controlPanel.right
        anchors.leftMargin: 20
        anchors.bottom: parent.bottom
        anchors.margins: 20
    }
}
