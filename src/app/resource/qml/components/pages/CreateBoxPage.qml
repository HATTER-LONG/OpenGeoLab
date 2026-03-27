import QtQuick
import QtQuick.Layouts
import "../.."
import ".."

FunctionPageBase {
    id: root

    pageTitle: qsTr("Create Box")
    pageIcon: "cubeOutline"
    actionId: "addBox"

    property string boxName: ""
    property real originX: 0.0
    property real originY: 0.0
    property real originZ: 0.0
    property real dimW: 10.0
    property real dimH: 10.0
    property real dimD: 10.0

    function getParameters() {
        return {
            module: "geometry",
            action: "create_box",
            param: {
                name: root.boxName,
                x: root.originX,
                y: root.originY,
                z: root.originZ,
                width: root.dimW,
                height: root.dimH,
                depth: root.dimD
            },
            mute: false
        };
    }

    ParamField {
        width: parent.width
        label: qsTr("Box Name")
        placeholder: qsTr("Auto-generated if empty")
        value: root.boxName

        onValueEdited: function(newValue) {
            root.boxName = newValue;
        }
    }

    CoordinateField {
        width: parent.width
        label: qsTr("Origin Point")
        coordX: root.originX
        coordY: root.originY
        coordZ: root.originZ

        onCoordinateChanged: function(x, y, z) {
            root.originX = x;
            root.originY = y;
            root.originZ = z;
        }
    }

    Text {
        text: qsTr("Dimensions")
        color: MainPages.theme.textSecondary
        font.pixelSize: 12
    }

    RowLayout {
        width: parent.width
        spacing: 6

        DimensionInput {
            Layout.fillWidth: true
            label: "W"
            value: root.dimW
            accentColor: "#E53935"
            tooltipText: qsTr("Width")

            onValueEdited: function(newVal) {
                root.dimW = newVal;
            }
        }

        DimensionInput {
            Layout.fillWidth: true
            label: "H"
            value: root.dimH
            accentColor: "#43A047"
            tooltipText: qsTr("Height")

            onValueEdited: function(newVal) {
                root.dimH = newVal;
            }
        }

        DimensionInput {
            Layout.fillWidth: true
            label: "D"
            value: root.dimD
            accentColor: "#1E88E5"
            tooltipText: qsTr("Depth")

            onValueEdited: function(newVal) {
                root.dimD = newVal;
            }
        }
    }

    Rectangle {
        width: parent.width
        height: infoRow.implicitHeight + 16
        radius: MainPages.theme.radiusSmall
        color: MainPages.theme.surfaceMuted

        RowLayout {
            id: infoRow

            anchors.fill: parent
            anchors.margins: 8
            spacing: 4

            Text {
                Layout.preferredWidth: 56
                text: qsTr("Volume:")
                color: MainPages.theme.textSecondary
                font.pixelSize: 11
            }

            Text {
                Layout.fillWidth: true
                text: (root.dimW * root.dimH * root.dimD).toFixed(3)
                color: MainPages.theme.textPrimary
                font.pixelSize: 11
                font.bold: true
                horizontalAlignment: Text.AlignLeft
            }
        }
    }
}
