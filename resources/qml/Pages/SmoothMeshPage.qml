/**
 * @file SmoothMeshPage.qml
 * @brief Function page for mesh smoothing
 *
 * Allows user to configure mesh smoothing parameters.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

FunctionPageBase {
    id: root

    pageTitle: qsTr("Smooth Mesh")
    pageIcon: "qrc:/opengeolab/resources/icons/smooth_mesh.svg"
    serviceName: "MeshService"
    actionId: "smoothMesh"

    width: 320

    // =========================================================
    // Parameters
    // =========================================================

    /// Target mesh ID
    property string targetMesh: ""
    /// Smoothing iterations
    property int iterations: 3
    /// Smoothing factor (0-1)
    property real factor: 0.5
    /// Preserve boundaries
    property bool preserveBoundaries: true
    /// Smoothing method
    property string method: "laplacian"

    function getParameters(): var {
        return {
            "action": "smoothMesh",
            "target": targetMesh,
            "iterations": iterations,
            "factor": factor,
            "method": method,
            "preserveBoundaries": preserveBoundaries
        };
    }

    // =========================================================
    // Content
    // =========================================================

    Column {
        width: parent.width
        spacing: 12

        // Target mesh selection
        Column {
            width: parent.width
            spacing: 4

            Label {
                text: qsTr("Target Mesh")
                font.pixelSize: 11
                color: Theme.textSecondary
            }

            Rectangle {
                width: parent.width
                height: 32
                radius: 4
                color: Theme.surface
                border.width: 1
                border.color: Theme.border

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 8

                    Label {
                        text: root.targetMesh || qsTr("Click to select...")
                        font.pixelSize: 12
                        color: root.targetMesh ? Theme.textPrimary : Theme.textDisabled
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }

                    AbstractButton {
                        id: pickMeshBtn
                        implicitWidth: 24
                        implicitHeight: 24
                        hoverEnabled: true

                        background: Rectangle {
                            radius: 4
                            color: pickMeshBtn.hovered ? Theme.hovered : "transparent"
                        }

                        contentItem: Label {
                            text: "🎯"
                            font.pixelSize: 14
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        ToolTip.visible: pickMeshBtn.hovered
                        ToolTip.text: qsTr("Pick from viewport")
                        ToolTip.delay: 500
                    }
                }
            }
        }

        // Smoothing method
        Column {
            width: parent.width
            spacing: 4

            Label {
                text: qsTr("Smoothing Method")
                font.pixelSize: 11
                color: Theme.textSecondary
            }

            RowLayout {
                width: parent.width
                spacing: 8

                MethodButton {
                    Layout.fillWidth: true
                    text: qsTr("Laplacian")
                    selected: root.method === "laplacian"
                    onClicked: root.method = "laplacian"
                }

                MethodButton {
                    Layout.fillWidth: true
                    text: qsTr("Taubin")
                    selected: root.method === "taubin"
                    onClicked: root.method = "taubin"
                }
            }
        }

        // Iterations
        Column {
            width: parent.width
            spacing: 4

            RowLayout {
                width: parent.width

                Label {
                    text: qsTr("Iterations")
                    font.pixelSize: 11
                    color: Theme.textSecondary
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: root.iterations.toString()
                    font.pixelSize: 11
                    font.bold: true
                    color: Theme.textPrimary
                }
            }

            Slider {
                id: iterSlider
                width: parent.width
                from: 1
                to: 20
                value: root.iterations
                stepSize: 1

                onValueChanged: root.iterations = Math.round(value)

                background: Rectangle {
                    x: iterSlider.leftPadding
                    y: iterSlider.topPadding + iterSlider.availableHeight / 2 - height / 2
                    implicitWidth: 200
                    implicitHeight: 4
                    width: iterSlider.availableWidth
                    height: implicitHeight
                    radius: 2
                    color: Theme.border

                    Rectangle {
                        width: iterSlider.visualPosition * parent.width
                        height: parent.height
                        color: Theme.accent
                        radius: 2
                    }
                }

                handle: Rectangle {
                    x: iterSlider.leftPadding + iterSlider.visualPosition * (iterSlider.availableWidth - width)
                    y: iterSlider.topPadding + iterSlider.availableHeight / 2 - height / 2
                    implicitWidth: 16
                    implicitHeight: 16
                    radius: 8
                    color: iterSlider.pressed ? Qt.darker(Theme.accent, 1.2) : Theme.accent
                    border.width: 2
                    border.color: Theme.white
                }
            }
        }

        // Smoothing factor
        Column {
            width: parent.width
            spacing: 4

            RowLayout {
                width: parent.width

                Label {
                    text: qsTr("Smoothing Factor")
                    font.pixelSize: 11
                    color: Theme.textSecondary
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: root.factor.toFixed(2)
                    font.pixelSize: 11
                    font.bold: true
                    color: Theme.textPrimary
                }
            }

            Slider {
                id: factorSlider
                width: parent.width
                from: 0.0
                to: 1.0
                value: root.factor
                stepSize: 0.05

                onValueChanged: root.factor = value

                background: Rectangle {
                    x: factorSlider.leftPadding
                    y: factorSlider.topPadding + factorSlider.availableHeight / 2 - height / 2
                    implicitWidth: 200
                    implicitHeight: 4
                    width: factorSlider.availableWidth
                    height: implicitHeight
                    radius: 2
                    color: Theme.border

                    Rectangle {
                        width: factorSlider.visualPosition * parent.width
                        height: parent.height
                        color: Theme.accent
                        radius: 2
                    }
                }

                handle: Rectangle {
                    x: factorSlider.leftPadding + factorSlider.visualPosition * (factorSlider.availableWidth - width)
                    y: factorSlider.topPadding + factorSlider.availableHeight / 2 - height / 2
                    implicitWidth: 16
                    implicitHeight: 16
                    radius: 8
                    color: factorSlider.pressed ? Qt.darker(Theme.accent, 1.2) : Theme.accent
                    border.width: 2
                    border.color: Theme.white
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
                    id: preserveCheck
                    checked: root.preserveBoundaries
                    onCheckedChanged: root.preserveBoundaries = checked

                    indicator: Rectangle {
                        implicitWidth: 16
                        implicitHeight: 16
                        radius: 3
                        color: preserveCheck.checked ? Theme.accent : Theme.surface
                        border.width: 1
                        border.color: preserveCheck.checked ? Theme.accent : Theme.border

                        Label {
                            anchors.centerIn: parent
                            text: "✓"
                            font.pixelSize: 11
                            color: Theme.white
                            visible: preserveCheck.checked
                        }
                    }
                }

                Label {
                    text: qsTr("Preserve boundary vertices")
                    font.pixelSize: 11
                    color: Theme.textPrimary
                    Layout.fillWidth: true

                    MouseArea {
                        anchors.fill: parent
                        onClicked: preserveCheck.toggle()
                    }
                }
            }
        }
    }

    // =========================================================
    // Method button component
    // =========================================================
    component MethodButton: AbstractButton {
        id: methodBtn

        property bool selected: false

        implicitHeight: 28
        hoverEnabled: true

        background: Rectangle {
            radius: 4
            color: methodBtn.selected ? Theme.accent : methodBtn.pressed ? Theme.clicked : methodBtn.hovered ? Theme.hovered : Theme.surface
            border.width: 1
            border.color: methodBtn.selected ? Theme.accent : Theme.border
        }

        contentItem: Label {
            text: methodBtn.text
            font.pixelSize: 11
            font.bold: methodBtn.selected
            color: methodBtn.selected ? Theme.white : Theme.textPrimary
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
