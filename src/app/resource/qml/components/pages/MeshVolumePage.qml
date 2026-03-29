import QtQuick
import QtQuick.Layouts
import "../.."
import ".."

FunctionPageBase {
    id: root

    pageTitle: qsTr("Volume Mesh (3D)")
    pageIcon: "meshVolume"
    actionId: "meshVolume"

    property string meshName: ""
    property int shapeId: -1
    property real minSize: 0.1
    property real maxSize: 10.0
    property int algorithm: 1
    property bool hexDominant: false
    property int meshOrder: 1
    property bool optimize: true
    property int optimizeAlgorithm: 0

    function getParameters() {
        return {
            module: "mesh",
            action: "generate_volume_mesh",
            param: {
                shapeId: root.shapeId,
                name: root.meshName,
                minSize: root.minSize,
                maxSize: root.maxSize,
                algorithm: root.algorithm,
                hexDominant: root.hexDominant,
                order: root.meshOrder,
                optimize: root.optimize,
                optimizeAlgorithm: root.optimizeAlgorithm
            },
            mute: false
        };
    }

    ParamField {
        width: parent.width
        theme: root.theme
        label: qsTr("Mesh Name")
        placeholder: qsTr("Auto-generated if empty")
        value: root.meshName

        onValueEdited: function(newValue) {
            root.meshName = newValue;
        }
    }

    ShapeSelector {
        width: parent.width
        theme: root.theme
        label: qsTr("Target Shape")
        selectedShapeId: root.shapeId

        onShapeSelected: function(sid) {
            root.shapeId = sid;
        }
    }

    Text {
        text: qsTr("Element Size")
        color: root.theme.textSecondary
        font.pixelSize: 12
    }

    RowLayout {
        width: parent.width
        spacing: 6

        DimensionInput {
            Layout.fillWidth: true
            theme: root.theme
            label: qsTr("Min")
            value: root.minSize
            minValue: 0.001
            accentColor: root.theme.accentB
            tooltipText: qsTr("Minimum element size")

            onValueEdited: function(newVal) {
                root.minSize = newVal;
            }
        }

        DimensionInput {
            Layout.fillWidth: true
            theme: root.theme
            label: qsTr("Max")
            value: root.maxSize
            minValue: 0.001
            accentColor: root.theme.accentA
            tooltipText: qsTr("Maximum element size")

            onValueEdited: function(newVal) {
                root.maxSize = newVal;
            }
        }
    }

    Text {
        text: qsTr("Volume Algorithm")
        color: root.theme.textSecondary
        font.pixelSize: 12
    }

    Rectangle {
        width: parent.width
        height: 28
        radius: root.theme.radiusSmall
        color: root.theme.surface
        border.color: volAlgoArea.containsMouse ? root.theme.accentB : root.theme.borderSubtle
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8

            Text {
                Layout.fillWidth: true
                text: {
                    switch (root.algorithm) {
                    case 1: return "Delaunay"
                    case 4: return "Frontal"
                    case 7: return "MMG3D"
                    case 10: return "HXT"
                    default: return "Delaunay"
                    }
                }
                color: root.theme.textPrimary
                font.pixelSize: 12
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                text: "\u25BE"
                color: root.theme.textSecondary
                font.pixelSize: 10
            }
        }

        MouseArea {
            id: volAlgoArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: volAlgoMenu.visible = !volAlgoMenu.visible
        }
    }

    Rectangle {
        id: volAlgoMenu
        width: parent.width
        visible: false
        height: volAlgoCol.implicitHeight + 8
        radius: root.theme.radiusSmall
        color: root.theme.surface
        border.color: root.theme.borderSubtle
        border.width: 1
        z: 100

        Column {
            id: volAlgoCol
            anchors.fill: parent
            anchors.margins: 4
            spacing: 2

            Repeater {
                model: [
                    { value: 1, label: "Delaunay" },
                    { value: 4, label: "Frontal" },
                    { value: 7, label: "MMG3D" },
                    { value: 10, label: "HXT" }
                ]

                Rectangle {
                    width: volAlgoCol.width
                    height: 24
                    radius: 2
                    color: volAlgoItemArea.containsMouse
                           ? root.theme.surfaceMuted
                           : (root.algorithm === modelData.value ? root.theme.surfaceMuted : "transparent")

                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        text: modelData.label
                        color: root.theme.textPrimary
                        font.pixelSize: 12
                        verticalAlignment: Text.AlignVCenter
                    }

                    MouseArea {
                        id: volAlgoItemArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            root.algorithm = modelData.value
                            volAlgoMenu.visible = false
                        }
                    }
                }
            }
        }
    }

    RowLayout {
        width: parent.width
        spacing: 6

        Text {
            text: qsTr("Element Type")
            color: root.theme.textSecondary
            font.pixelSize: 12
            Layout.preferredWidth: 80
        }

        Rectangle {
            Layout.fillWidth: true
            height: 28
            radius: root.theme.radiusSmall
            color: root.theme.surface
            border.color: volTypeArea.containsMouse ? root.theme.accentB : root.theme.borderSubtle
            border.width: 1

            Text {
                anchors.fill: parent
                anchors.leftMargin: 8
                text: root.hexDominant ? qsTr("Hex") : qsTr("Tetrahedron")
                color: root.theme.textPrimary
                font.pixelSize: 12
                verticalAlignment: Text.AlignVCenter
            }

            MouseArea {
                id: volTypeArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: root.hexDominant = !root.hexDominant
            }
        }
    }

    RowLayout {
        width: parent.width
        spacing: 6

        Text {
            text: qsTr("Order")
            color: root.theme.textSecondary
            font.pixelSize: 12
            Layout.preferredWidth: 80
        }

        Rectangle {
            Layout.fillWidth: true
            height: 28
            radius: root.theme.radiusSmall
            color: root.theme.surface
            border.color: volOrderArea.containsMouse ? root.theme.accentB : root.theme.borderSubtle
            border.width: 1

            Text {
                anchors.fill: parent
                anchors.leftMargin: 8
                text: root.meshOrder === 1 ? qsTr("1st (Linear)") : qsTr("2nd (Quadratic)")
                color: root.theme.textPrimary
                font.pixelSize: 12
                verticalAlignment: Text.AlignVCenter
            }

            MouseArea {
                id: volOrderArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: root.meshOrder = (root.meshOrder === 1) ? 2 : 1
            }
        }
    }

    RowLayout {
        width: parent.width
        spacing: 6

        Text {
            text: qsTr("Optimize")
            color: root.theme.textSecondary
            font.pixelSize: 12
            Layout.preferredWidth: 80
        }

        Rectangle {
            width: 36
            height: 20
            radius: 10
            color: root.optimize ? root.theme.accentB : root.theme.surfaceMuted
            border.color: root.theme.borderSubtle
            border.width: 1

            Behavior on color { ColorAnimation { duration: 150 } }

            Rectangle {
                width: 16
                height: 16
                radius: 8
                x: root.optimize ? parent.width - width - 2 : 2
                anchors.verticalCenter: parent.verticalCenter
                color: "white"

                Behavior on x { NumberAnimation { duration: 150 } }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.optimize = !root.optimize
            }
        }
    }

    Text {
        text: qsTr("Optimize Algorithm")
        color: root.theme.textSecondary
        font.pixelSize: 12
    }

    Rectangle {
        width: parent.width
        height: 28
        radius: root.theme.radiusSmall
        color: root.theme.surface
        border.color: optAlgoArea.containsMouse ? root.theme.accentB : root.theme.borderSubtle
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8

            Text {
                Layout.fillWidth: true
                text: {
                    switch (root.optimizeAlgorithm) {
                    case 0: return qsTr("Default")
                    case 1: return "Netgen"
                    case 2: return qsTr("High Order")
                    default: return qsTr("Default")
                    }
                }
                color: root.theme.textPrimary
                font.pixelSize: 12
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                text: "\u25BE"
                color: root.theme.textSecondary
                font.pixelSize: 10
            }
        }

        MouseArea {
            id: optAlgoArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: optAlgoMenu.visible = !optAlgoMenu.visible
        }
    }

    Rectangle {
        id: optAlgoMenu
        width: parent.width
        visible: false
        height: optAlgoCol.implicitHeight + 8
        radius: root.theme.radiusSmall
        color: root.theme.surface
        border.color: root.theme.borderSubtle
        border.width: 1
        z: 100

        Column {
            id: optAlgoCol
            anchors.fill: parent
            anchors.margins: 4
            spacing: 2

            Repeater {
                model: [
                    { value: 0, label: qsTr("Default") },
                    { value: 1, label: "Netgen" },
                    { value: 2, label: qsTr("High Order") }
                ]

                Rectangle {
                    width: optAlgoCol.width
                    height: 24
                    radius: 2
                    color: optAlgoItemArea.containsMouse
                           ? root.theme.surfaceMuted
                           : (root.optimizeAlgorithm === modelData.value ? root.theme.surfaceMuted : "transparent")

                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        text: modelData.label
                        color: root.theme.textPrimary
                        font.pixelSize: 12
                        verticalAlignment: Text.AlignVCenter
                    }

                    MouseArea {
                        id: optAlgoItemArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            root.optimizeAlgorithm = modelData.value
                            optAlgoMenu.visible = false
                        }
                    }
                }
            }
        }
    }
}
