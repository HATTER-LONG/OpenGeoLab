pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import OpenGeoLab 1.0 as OGL

/**
 * @file ModelTreeSidebar.qml
 * @brief Model tree sidebar displaying geometry hierarchy
 *
 * Shows hierarchical structure of loaded models including:
 * - Parts with their names
 * - Solid counts
 * - Face, edge, and vertex counts
 *
 * @note Displays placeholder content when no model is loaded.
 */
Item {
    id: root

    Rectangle {
        anchors.fill: parent
        color: Theme.surfaceColor
        border.width: 1
        border.color: Theme.borderColor

        Column {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 4

            // Header
            Rectangle {
                width: parent.width
                height: 32
                color: Theme.surfaceAltColor
                radius: 4

                Label {
                    anchors.centerIn: parent
                    text: qsTr("Model Tree")
                    font.bold: true
                    font.pixelSize: 13
                    color: Theme.textPrimaryColor
                }
            }

            // Tree view - displays actual model data
            ScrollView {
                width: parent.width
                height: parent.height - 36
                clip: true

                Column {
                    width: parent.width
                    spacing: 2

                    // Dynamically generate parts from ModelManager
                    Repeater {
                        model: OGL.ModelManager.parts

                        Column {
                            id: partColumn
                            width: parent.width
                            required property var modelData

                            // Part header
                            TreeItem {
                                text: partColumn.modelData.name
                                icon: "📦"
                                indent: 0
                            }

                            // Part details
                            TreeItem {
                                text: qsTr("Solids: %1").arg(partColumn.modelData.solidCount)
                                icon: "🧊"
                                indent: 1
                            }

                            TreeItem {
                                text: qsTr("Faces: %1").arg(partColumn.modelData.faceCount)
                                icon: "▫"
                                indent: 1
                            }

                            TreeItem {
                                text: qsTr("Edges: %1").arg(partColumn.modelData.edgeCount)
                                icon: "―"
                                indent: 1
                            }

                            TreeItem {
                                text: qsTr("Vertices: %1").arg(partColumn.modelData.vertexCount)
                                icon: "•"
                                indent: 1
                            }
                        }
                    }

                    // Placeholder when no model loaded
                    Rectangle {
                        visible: !OGL.ModelManager.hasModel
                        width: parent.width
                        height: 60
                        color: Theme.surfaceAltColor
                        radius: 4
                        anchors.margins: 8

                        Column {
                            anchors.centerIn: parent
                            spacing: 4

                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "📂"
                                font.pixelSize: 24
                            }

                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: qsTr("No model loaded")
                                font.pixelSize: 11
                                color: Theme.textSecondaryColor
                            }
                        }
                    }
                }
            }
        }
    }

    /**
     * @brief Tree item component for displaying hierarchy entries
     */
    component TreeItem: Rectangle {
        id: treeItem

        /**
         * @brief Display text for the item
         */
        property string text: ""

        /**
         * @brief Icon emoji/character for the item
         */
        property string icon: ""

        /**
         * @brief Indentation level (0 = root)
         */
        property int indent: 0

        width: parent ? parent.width : 200
        height: 24
        color: itemMouseArea.containsMouse ? Theme.ribbonHoverColor : "transparent"
        radius: 3

        Row {
            anchors.fill: parent
            anchors.leftMargin: 8 + (treeItem.indent * 16)
            spacing: 6

            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: treeItem.icon
                font.pixelSize: 12
            }

            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: treeItem.text
                font.pixelSize: 12
                color: Theme.textPrimaryColor
            }
        }

        MouseArea {
            id: itemMouseArea
            anchors.fill: parent
            hoverEnabled: true
        }
    }
}
