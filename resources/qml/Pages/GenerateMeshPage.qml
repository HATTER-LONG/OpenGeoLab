/**
 * @file GenerateMeshPage.qml
 * @brief Function page for mesh generation
 *
 * Allows user to configure mesh generation parameters.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

FunctionPageBase {
    id: root

    pageTitle: qsTr("Generate Mesh")
    pageIcon: "qrc:/opengeolab/resources/icons/mesh.svg"
    serviceName: "MeshService"
    actionId: "generateMesh"

    width: 340

    // =========================================================
    // Parameters
    // =========================================================

    /// Target geometry IDs
    property var targetGeometries: []
    /// Mesh element size
    property real elementSize: 1.0
    /// Mesh quality (0-1)
    property real quality: 0.8
    /// Mesh type: "triangular", "quadrilateral", "tetrahedral"
    property string meshType: "triangular"
    /// Enable adaptive meshing
    property bool adaptive: true

    function getParameters(): var {
        return {
            "action": "generateMesh",
            "targets": targetGeometries,
            "elementSize": elementSize,
            "quality": quality,
            "meshType": meshType,
            "adaptive": adaptive
        };
    }

    // =========================================================
    // Content
    // =========================================================

    Column {
        width: parent.width
        spacing: 12

        // Target selection info
        Rectangle {
            width: parent.width
            height: targetRow.implicitHeight + 12
            radius: 4
            color: Theme.surfaceAlt
            border.width: 1
            border.color: Theme.border

            RowLayout {
                id: targetRow
                anchors.fill: parent
                anchors.margins: 6
                spacing: 8

                Label {
                    text: "📐"
                    font.pixelSize: 16
                }

                Label {
                    text: root.targetGeometries.length > 0 ? qsTr("%1 geometries selected").arg(root.targetGeometries.length) : qsTr("Select geometries in viewport")
                    font.pixelSize: 11
                    color: root.targetGeometries.length > 0 ? Theme.textPrimary : Theme.textDisabled
                    Layout.fillWidth: true
                }

                AbstractButton {
                    id: pickGeometryBtn
                    implicitWidth: 24
                    implicitHeight: 24
                    hoverEnabled: true

                    background: Rectangle {
                        radius: 4
                        color: pickGeometryBtn.hovered ? Theme.hovered : "transparent"
                    }

                    contentItem: Label {
                        text: "🎯"
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    ToolTip.visible: pickGeometryBtn.hovered
                    ToolTip.text: qsTr("Select from viewport")
                    ToolTip.delay: 500
                }
            }
        }

        // Element size
        Column {
            width: parent.width
            spacing: 4

            RowLayout {
                width: parent.width

                Label {
                    text: qsTr("Element Size")
                    font.pixelSize: 11
                    color: Theme.textSecondary
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: root.elementSize.toFixed(2)
                    font.pixelSize: 11
                    font.bold: true
                    color: Theme.textPrimary
                }
            }

            Slider {
                id: sizeSlider
                width: parent.width
                from: 0.1
                to: 10.0
                value: root.elementSize
                stepSize: 0.1

                onValueChanged: root.elementSize = value

                background: Rectangle {
                    x: sizeSlider.leftPadding
                    y: sizeSlider.topPadding + sizeSlider.availableHeight / 2 - height / 2
                    implicitWidth: 200
                    implicitHeight: 4
                    width: sizeSlider.availableWidth
                    height: implicitHeight
                    radius: 2
                    color: Theme.border

                    Rectangle {
                        width: sizeSlider.visualPosition * parent.width
                        height: parent.height
                        color: Theme.accent
                        radius: 2
                    }
                }

                handle: Rectangle {
                    x: sizeSlider.leftPadding + sizeSlider.visualPosition * (sizeSlider.availableWidth - width)
                    y: sizeSlider.topPadding + sizeSlider.availableHeight / 2 - height / 2
                    implicitWidth: 16
                    implicitHeight: 16
                    radius: 8
                    color: sizeSlider.pressed ? Qt.darker(Theme.accent, 1.2) : Theme.accent
                    border.width: 2
                    border.color: Theme.white
                }
            }
        }

        // Quality
        Column {
            width: parent.width
            spacing: 4

            RowLayout {
                width: parent.width

                Label {
                    text: qsTr("Mesh Quality")
                    font.pixelSize: 11
                    color: Theme.textSecondary
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: Math.round(root.quality * 100) + "%"
                    font.pixelSize: 11
                    font.bold: true
                    color: root.quality > 0.7 ? Theme.success : root.quality > 0.4 ? Theme.accent : Theme.danger
                }
            }

            Slider {
                id: qualitySlider
                width: parent.width
                from: 0.1
                to: 1.0
                value: root.quality
                stepSize: 0.05

                onValueChanged: root.quality = value

                background: Rectangle {
                    x: qualitySlider.leftPadding
                    y: qualitySlider.topPadding + qualitySlider.availableHeight / 2 - height / 2
                    implicitWidth: 200
                    implicitHeight: 4
                    width: qualitySlider.availableWidth
                    height: implicitHeight
                    radius: 2
                    color: Theme.border

                    Rectangle {
                        width: qualitySlider.visualPosition * parent.width
                        height: parent.height
                        color: root.quality > 0.7 ? Theme.success : root.quality > 0.4 ? Theme.accent : Theme.danger
                        radius: 2
                    }
                }

                handle: Rectangle {
                    x: qualitySlider.leftPadding + qualitySlider.visualPosition * (qualitySlider.availableWidth - width)
                    y: qualitySlider.topPadding + qualitySlider.availableHeight / 2 - height / 2
                    implicitWidth: 16
                    implicitHeight: 16
                    radius: 8
                    color: qualitySlider.pressed ? Theme.clicked : Theme.surface
                    border.width: 2
                    border.color: root.quality > 0.7 ? Theme.success : root.quality > 0.4 ? Theme.accent : Theme.danger
                }
            }
        }

        // Mesh type
        Column {
            width: parent.width
            spacing: 4

            Label {
                text: qsTr("Mesh Type")
                font.pixelSize: 11
                color: Theme.textSecondary
            }

            RowLayout {
                width: parent.width
                spacing: 6

                MeshTypeButton {
                    Layout.fillWidth: true
                    text: qsTr("Tri")
                    iconText: "△"
                    selected: root.meshType === "triangular"
                    onClicked: root.meshType = "triangular"
                }

                MeshTypeButton {
                    Layout.fillWidth: true
                    text: qsTr("Quad")
                    iconText: "□"
                    selected: root.meshType === "quadrilateral"
                    onClicked: root.meshType = "quadrilateral"
                }

                MeshTypeButton {
                    Layout.fillWidth: true
                    text: qsTr("Tet")
                    iconText: "⬡"
                    selected: root.meshType === "tetrahedral"
                    onClicked: root.meshType = "tetrahedral"
                }
            }
        }

        // Options
        Rectangle {
            width: parent.width
            height: optionRow.implicitHeight + 12
            radius: 4
            color: Theme.surfaceAlt

            RowLayout {
                id: optionRow
                anchors.fill: parent
                anchors.margins: 6
                spacing: 8

                CheckBox {
                    id: adaptiveCheck
                    checked: root.adaptive
                    onCheckedChanged: root.adaptive = checked

                    indicator: Rectangle {
                        implicitWidth: 16
                        implicitHeight: 16
                        radius: 3
                        color: adaptiveCheck.checked ? Theme.accent : Theme.surface
                        border.width: 1
                        border.color: adaptiveCheck.checked ? Theme.accent : Theme.border

                        Label {
                            anchors.centerIn: parent
                            text: "✓"
                            font.pixelSize: 11
                            color: Theme.white
                            visible: adaptiveCheck.checked
                        }
                    }
                }

                Label {
                    text: qsTr("Enable adaptive meshing")
                    font.pixelSize: 11
                    color: Theme.textPrimary
                    Layout.fillWidth: true

                    MouseArea {
                        anchors.fill: parent
                        onClicked: adaptiveCheck.toggle()
                    }
                }
            }
        }
    }

    // =========================================================
    // Mesh type button component
    // =========================================================
    component MeshTypeButton: AbstractButton {
        id: meshBtn

        property bool selected: false
        property string iconText: ""

        implicitHeight: 36
        hoverEnabled: true

        background: Rectangle {
            radius: 4
            color: meshBtn.selected ? Theme.accent : meshBtn.pressed ? Theme.clicked : meshBtn.hovered ? Theme.hovered : Theme.surface
            border.width: 1
            border.color: meshBtn.selected ? Theme.accent : Theme.border
        }

        contentItem: Column {
            spacing: 2

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: meshBtn.iconText
                font.pixelSize: 14
                color: meshBtn.selected ? Theme.white : Theme.textPrimary
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: meshBtn.text
                font.pixelSize: 10
                font.bold: meshBtn.selected
                color: meshBtn.selected ? Theme.white : Theme.textPrimary
            }
        }
    }
}
