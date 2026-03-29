import QtQuick
import QtQuick.Layouts
import "../.."
import ".."

FunctionPageBase {
    id: root

    pageTitle: qsTr("Surface Mesh (2D)")
    pageIcon: "meshSurface"
    actionId: "meshSurface"

    property string meshName: ""
    property int shapeId: -1
    property real minSize: 0.1
    property real maxSize: 10.0
    property int algorithm: 6
    property bool quadDominant: false
    property int meshOrder: 1
    property bool optimize: true

    function getParameters() {
        return {
            module: "mesh",
            action: "generate_surface_mesh",
            param: {
                shapeId: root.shapeId,
                name: root.meshName,
                minSize: root.minSize,
                maxSize: root.maxSize,
                algorithm: root.algorithm,
                quadDominant: root.quadDominant,
                order: root.meshOrder,
                optimize: root.optimize
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
        text: qsTr("Algorithm")
        color: root.theme.textSecondary
        font.pixelSize: 12
    }

    Rectangle {
        width: parent.width
        height: 28
        radius: root.theme.radiusSmall
        color: root.theme.surface
        border.color: algoArea.containsMouse ? root.theme.accentB : root.theme.borderSubtle
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8

            Text {
                Layout.fillWidth: true
                text: {
                    switch (root.algorithm) {
                    case 1: return "MeshAdapt"
                    case 5: return "Delaunay"
                    case 6: return "Frontal-Delaunay"
                    case 7: return "BAMG"
                    default: return "Frontal-Delaunay"
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
            id: algoArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: algoMenu.visible = !algoMenu.visible
        }
    }

    Rectangle {
        id: algoMenu
        width: parent.width
        visible: false
        height: algoCol.implicitHeight + 8
        radius: root.theme.radiusSmall
        color: root.theme.surface
        border.color: root.theme.borderSubtle
        border.width: 1
        z: 100

        Column {
            id: algoCol
            anchors.fill: parent
            anchors.margins: 4
            spacing: 2

            Repeater {
                model: [
                    { value: 1, label: "MeshAdapt" },
                    { value: 5, label: "Delaunay" },
                    { value: 6, label: "Frontal-Delaunay" },
                    { value: 7, label: "BAMG" }
                ]

                Rectangle {
                    width: algoCol.width
                    height: 24
                    radius: 2
                    color: algoItemArea.containsMouse
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
                        id: algoItemArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            root.algorithm = modelData.value
                            algoMenu.visible = false
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
            border.color: typeArea.containsMouse ? root.theme.accentB : root.theme.borderSubtle
            border.width: 1

            Text {
                anchors.fill: parent
                anchors.leftMargin: 8
                text: root.quadDominant ? qsTr("Quad") : qsTr("Triangle")
                color: root.theme.textPrimary
                font.pixelSize: 12
                verticalAlignment: Text.AlignVCenter
            }

            MouseArea {
                id: typeArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: root.quadDominant = !root.quadDominant
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
            border.color: orderArea.containsMouse ? root.theme.accentB : root.theme.borderSubtle
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
                id: orderArea
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
}
