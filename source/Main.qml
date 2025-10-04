// Copyright (C) 2025 OpenGeoLab
// SPDX-License-Identifier: MIT

// Allow delegate access to outer ids
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import OpenGeoLab

Window {
    id: root
    visible: true
    width: 960
    height: 600
    title: "OpenGeoLab - Triangle Demo"

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Left control panel (1/4 width)
        Rectangle {
            Layout.preferredWidth: parent.width * 0.25
            Layout.fillHeight: true
            color: "#2b2b2b"
            border.color: "#444"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 14

                Label {
                    text: "Triangle Controls"
                    font.pixelSize: 20
                    font.bold: true
                    color: "white"
                }

                Rectangle {
                    implicitHeight: 1
                    color: "#555"
                    Layout.fillWidth: true
                }

                // Color selection section
                Label {
                    text: "Colors"
                    color: "#ccc"
                    font.pixelSize: 14
                }

                GridLayout {
                    columns: 3
                    rowSpacing: 8
                    columnSpacing: 8
                    Layout.fillWidth: true

                    Repeater {
                        model: ["red", "green", "blue", "yellow", "magenta", "cyan"]
                        delegate: Button {
                            required property string modelData
                            required property int index

                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            text: modelData.charAt(0).toUpperCase() + modelData.slice(1)

                            ToolTip.visible: hovered
                            ToolTip.text: "Set color to " + modelData

                            onClicked: {
                                triangle.color = modelData;
                            }

                            background: Rectangle {
                                color: parent.down ? "#555" : (parent.hovered ? "#444" : "#333")
                                border.color: triangle.color === modelData ? "#0078d7" : "#666"
                                border.width: triangle.color === modelData ? 2 : 1
                                radius: 4
                            }

                            contentItem: Text {
                                text: parent.text
                                font: parent.font
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }

                Rectangle {
                    implicitHeight: 1
                    color: "#555"
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                }

                // Rotation control section
                Label {
                    text: "Rotation"
                    color: "#ccc"
                    font.pixelSize: 14
                }

                Slider {
                    id: angleSlider
                    Layout.fillWidth: true
                    from: 0
                    to: 360
                    value: triangle.angle
                    stepSize: 1

                    onValueChanged: {
                        triangle.angle = value;
                    }

                    background: Rectangle {
                        x: angleSlider.leftPadding
                        y: angleSlider.topPadding + angleSlider.availableHeight / 2 - height / 2
                        width: angleSlider.availableWidth
                        height: 4
                        radius: 2
                        color: "#555"

                        Rectangle {
                            width: angleSlider.visualPosition * parent.width
                            height: parent.height
                            color: "#0078d7"
                            radius: 2
                        }
                    }

                    handle: Rectangle {
                        x: angleSlider.leftPadding + angleSlider.visualPosition * (angleSlider.availableWidth - width)
                        y: angleSlider.topPadding + angleSlider.availableHeight / 2 - height / 2
                        width: 20
                        height: 20
                        radius: 10
                        color: angleSlider.pressed ? "#0078d7" : "#ffffff"
                        border.color: "#0078d7"
                        border.width: 2
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: Math.round(triangle.angle) + "°"
                        color: "#ddd"
                        font.pixelSize: 16
                        font.bold: true
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Button {
                        text: "Reset"
                        onClicked: angleSlider.value = 0

                        background: Rectangle {
                            color: parent.down ? "#555" : (parent.hovered ? "#444" : "#333")
                            border.color: "#666"
                            radius: 4
                        }

                        contentItem: Text {
                            text: parent.text
                            font: parent.font
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
                }

                // Info label
                Label {
                    text: "OpenGL Triangle Demo\nBuilt with Qt Quick"
                    color: "#888"
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }
            }
        }

        // Right render area (3/4 width)
        Rectangle {
            id: renderArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "black"

            TriangleItem {
                id: triangle
                anchors.fill: parent
                color: "red"
                angle: 0
            }

            // Optional: FPS counter
            Text {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                text: "FPS: " + Math.round(1000 / frameTimer.avgInterval)
                color: "lime"
                font.pixelSize: 14
                font.family: "Consolas"

                Timer {
                    id: frameTimer
                    property real avgInterval: 16.67
                    running: true
                    repeat: true
                    interval: 16
                    onTriggered: {
                        frameTimer.avgInterval = frameTimer.avgInterval * 0.9 + 16 * 0.1;
                    }
                }
            }
        }
    }
}
