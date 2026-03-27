import QtQuick
import QtQuick.Layouts
import ".."

Item {
    id: root

    property string label: ""
    property real coordX: 0.0
    property real coordY: 0.0
    property real coordZ: 0.0
    property int decimals: 3

    signal coordinateChanged(real x, real y, real z)

    implicitWidth: 300
    implicitHeight: contentColumn.implicitHeight

    Column {
        id: contentColumn

        width: root.width > 0 ? root.width : root.implicitWidth
        spacing: 4

        Text {
            text: root.label
            visible: root.label.length > 0
            color: MainPages.theme.textSecondary
            font.pixelSize: 12
        }

        RowLayout {
            width: parent.width
            spacing: 6

            DimensionInput {
                Layout.fillWidth: true
                label: qsTr("X")
                value: root.coordX
                decimals: root.decimals
                minValue: -1e9
                accentColor: "#E53935"

                onValueEdited: function(newVal) {
                    root.coordX = newVal;
                    root.coordinateChanged(root.coordX, root.coordY, root.coordZ);
                }
            }

            DimensionInput {
                Layout.fillWidth: true
                label: qsTr("Y")
                value: root.coordY
                decimals: root.decimals
                minValue: -1e9
                accentColor: "#43A047"

                onValueEdited: function(newVal) {
                    root.coordY = newVal;
                    root.coordinateChanged(root.coordX, root.coordY, root.coordZ);
                }
            }

            DimensionInput {
                Layout.fillWidth: true
                label: qsTr("Z")
                value: root.coordZ
                decimals: root.decimals
                minValue: -1e9
                accentColor: "#1E88E5"

                onValueEdited: function(newVal) {
                    root.coordZ = newVal;
                    root.coordinateChanged(root.coordX, root.coordY, root.coordZ);
                }
            }
        }
    }
}
