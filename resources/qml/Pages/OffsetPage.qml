/**
 * @file OffsetPage.qml
 * @brief Function page for offsetting geometry
 *
 * Allows user to offset selected geometry by a specified distance.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

FunctionPageBase {
    id: root

    pageTitle: qsTr("Offset Geometry")
    pageIcon: "qrc:/opengeolab/resources/icons/offset.svg"
    serviceName: "GeometryService"
    actionId: "offset"

    width: 320

    // =========================================================
    // Parameters
    // =========================================================

    /// Selected geometry ID to offset
    property string targetGeometry: ""
    /// Offset distance
    property real distance: 1.0
    /// Offset direction: "outward" or "inward"
    property string direction: "outward"
    /// Number of copies (0 = replace original)
    property int copies: 1
    /// Keep original geometry
    property bool keepOriginal: true

    function getParameters(): var {
        return {
            "action": "offset",
            "target": targetGeometry,
            "distance": direction === "inward" ? -distance : distance,
            "copies": copies,
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
                        id: pickTargetBtn
                        implicitWidth: 24
                        implicitHeight: 24
                        hoverEnabled: true

                        background: Rectangle {
                            radius: 4
                            color: pickTargetBtn.hovered ? Theme.hovered : "transparent"
                        }

                        contentItem: Label {
                            text: "🎯"
                            font.pixelSize: 14
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        ToolTip.visible: pickTargetBtn.hovered
                        ToolTip.text: qsTr("Pick from viewport")
                        ToolTip.delay: 500
                    }
                }
            }
        }

        // Offset distance
        ParamField {
            label: qsTr("Offset Distance")
            inputType: "number"
            value: root.distance.toString()
            minValue: 0.001
            decimals: 3
            onValueEdited: newValue => root.distance = parseFloat(newValue) || 1.0
        }

        // Direction selection
        Column {
            width: parent.width
            spacing: 4

            Label {
                text: qsTr("Direction")
                font.pixelSize: 11
                color: Theme.textSecondary
            }

            RowLayout {
                width: parent.width
                spacing: 8

                DirectionButton {
                    Layout.fillWidth: true
                    text: qsTr("Outward")
                    iconText: "↗"
                    selected: root.direction === "outward"
                    onClicked: root.direction = "outward"
                }

                DirectionButton {
                    Layout.fillWidth: true
                    text: qsTr("Inward")
                    iconText: "↙"
                    selected: root.direction === "inward"
                    onClicked: root.direction = "inward"
                }
            }
        }

        // Number of copies
        Column {
            width: parent.width
            spacing: 4

            Label {
                text: qsTr("Number of Copies")
                font.pixelSize: 11
                color: Theme.textSecondary
            }

            RowLayout {
                width: parent.width
                spacing: 8

                SpinBox {
                    id: copiesSpinBox
                    Layout.fillWidth: true

                    from: 1
                    to: 100
                    value: root.copies
                    editable: true

                    onValueChanged: root.copies = value

                    background: Rectangle {
                        radius: 4
                        color: Theme.surface
                        border.width: copiesSpinBox.activeFocus ? 2 : 1
                        border.color: copiesSpinBox.activeFocus ? Theme.accent : Theme.border
                    }

                    contentItem: TextInput {
                        text: copiesSpinBox.displayText
                        font.pixelSize: 12
                        color: Theme.textPrimary
                        selectionColor: Theme.accent
                        selectedTextColor: Theme.white
                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter

                        readOnly: !copiesSpinBox.editable
                        validator: copiesSpinBox.validator
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                    }

                    up.indicator: Rectangle {
                        x: parent.width - width
                        height: parent.height / 2
                        implicitWidth: 24
                        implicitHeight: parent.height / 2
                        radius: 4
                        color: copiesSpinBox.up.pressed ? Theme.clicked : copiesSpinBox.up.hovered ? Theme.hovered : "transparent"

                        Label {
                            anchors.centerIn: parent
                            text: "▲"
                            font.pixelSize: 8
                            color: Theme.textPrimary
                        }
                    }

                    down.indicator: Rectangle {
                        x: parent.width - width
                        y: parent.height / 2
                        height: parent.height / 2
                        implicitWidth: 24
                        implicitHeight: parent.height / 2
                        radius: 4
                        color: copiesSpinBox.down.pressed ? Theme.clicked : copiesSpinBox.down.hovered ? Theme.hovered : "transparent"

                        Label {
                            anchors.centerIn: parent
                            text: "▼"
                            font.pixelSize: 8
                            color: Theme.textPrimary
                        }
                    }
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
    // Direction button component
    // =========================================================
    component DirectionButton: AbstractButton {
        id: dirBtn

        property bool selected: false
        property string iconText: ""

        implicitHeight: 32
        hoverEnabled: true

        background: Rectangle {
            radius: 4
            color: dirBtn.selected ? Theme.accent : dirBtn.pressed ? Theme.clicked : dirBtn.hovered ? Theme.hovered : Theme.surface
            border.width: 1
            border.color: dirBtn.selected ? Theme.accent : Theme.border
        }

        contentItem: RowLayout {
            spacing: 4

            Label {
                text: dirBtn.iconText
                font.pixelSize: 14
                color: dirBtn.selected ? Theme.white : Theme.textPrimary
                Layout.leftMargin: 8
            }

            Label {
                text: dirBtn.text
                font.pixelSize: 11
                font.bold: dirBtn.selected
                color: dirBtn.selected ? Theme.white : Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }
        }
    }
}
