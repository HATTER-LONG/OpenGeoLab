/**
 * @file TrimPage.qml
 * @brief Function page for trimming geometry
 *
 * Allows user to select geometry and define trim parameters.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

FunctionPageBase {
    id: root

    pageTitle: qsTr("Trim Geometry")
    pageIcon: "qrc:/opengeolab/resources/icons/trim.svg"
    serviceName: "GeometryService"
    actionId: "trim"

    width: 320

    // =========================================================
    // Parameters
    // =========================================================

    /// Selected geometry ID to trim
    property string targetGeometry: ""
    /// Trim boundary geometry ID
    property string boundaryGeometry: ""
    /// Trim mode: "inside" or "outside"
    property string trimMode: "outside"
    /// Keep original geometry
    property bool keepOriginal: false

    function getParameters() {
        return {
            "action": "trim",
            "target": targetGeometry,
            "boundary": boundaryGeometry,
            "mode": trimMode,
            "keepOriginal": keepOriginal
        };
    }

    // =========================================================
    // Content
    // =========================================================

    Column {
        width: parent.width
        spacing: 12

        // Target geometry selection
        Column {
            width: parent.width
            spacing: 4

            Label {
                text: qsTr("Target Geometry")
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
                        text: root.targetGeometry || qsTr("Click to select...")
                        font.pixelSize: 12
                        color: root.targetGeometry ? Theme.textPrimary : Theme.textDisabled
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }

                    AbstractButton {
                        implicitWidth: 24
                        implicitHeight: 24
                        hoverEnabled: true

                        background: Rectangle {
                            radius: 4
                            color: parent.hovered ? Theme.hovered : "transparent"
                        }

                        contentItem: Label {
                            text: "🎯"
                            font.pixelSize: 14
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Pick from viewport")
                        ToolTip.delay: 500
                    }
                }
            }
        }

        // Boundary geometry selection
        Column {
            width: parent.width
            spacing: 4

            Label {
                text: qsTr("Boundary Geometry")
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
                        text: root.boundaryGeometry || qsTr("Click to select...")
                        font.pixelSize: 12
                        color: root.boundaryGeometry ? Theme.textPrimary : Theme.textDisabled
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }

                    AbstractButton {
                        implicitWidth: 24
                        implicitHeight: 24
                        hoverEnabled: true

                        background: Rectangle {
                            radius: 4
                            color: parent.hovered ? Theme.hovered : "transparent"
                        }

                        contentItem: Label {
                            text: "🎯"
                            font.pixelSize: 14
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Pick from viewport")
                        ToolTip.delay: 500
                    }
                }
            }
        }

        // Trim mode selection
        Column {
            width: parent.width
            spacing: 4

            Label {
                text: qsTr("Trim Mode")
                font.pixelSize: 11
                color: Theme.textSecondary
            }

            RowLayout {
                width: parent.width
                spacing: 8

                ModeButton {
                    Layout.fillWidth: true
                    text: qsTr("Outside")
                    selected: root.trimMode === "outside"
                    onClicked: root.trimMode = "outside"
                }

                ModeButton {
                    Layout.fillWidth: true
                    text: qsTr("Inside")
                    selected: root.trimMode === "inside"
                    onClicked: root.trimMode = "inside"
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
                    id: keepOriginalCheck
                    checked: root.keepOriginal
                    onCheckedChanged: root.keepOriginal = checked

                    indicator: Rectangle {
                        implicitWidth: 16
                        implicitHeight: 16
                        radius: 3
                        color: keepOriginalCheck.checked ? Theme.accent : Theme.surface
                        border.width: 1
                        border.color: keepOriginalCheck.checked ? Theme.accent : Theme.border

                        Label {
                            anchors.centerIn: parent
                            text: "✓"
                            font.pixelSize: 11
                            color: Theme.white
                            visible: keepOriginalCheck.checked
                        }
                    }
                }

                Label {
                    text: qsTr("Keep original geometry")
                    font.pixelSize: 11
                    color: Theme.textPrimary
                    Layout.fillWidth: true

                    MouseArea {
                        anchors.fill: parent
                        onClicked: keepOriginalCheck.toggle()
                    }
                }
            }
        }
    }

    // =========================================================
    // Mode button component
    // =========================================================
    component ModeButton: AbstractButton {
        id: modeBtn

        property bool selected: false

        implicitHeight: 28
        hoverEnabled: true

        background: Rectangle {
            radius: 4
            color: modeBtn.selected ? Theme.accent :
                   modeBtn.pressed ? Theme.clicked :
                   modeBtn.hovered ? Theme.hovered : Theme.surface
            border.width: 1
            border.color: modeBtn.selected ? Theme.accent : Theme.border
        }

        contentItem: Label {
            text: modeBtn.text
            font.pixelSize: 11
            font.bold: modeBtn.selected
            color: modeBtn.selected ? Theme.white : Theme.textPrimary
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
