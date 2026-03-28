pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import ".."

Item {
    id: root

    property AppTheme theme: MainPages.theme
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
            color: root.theme.textSecondary
            font.pixelSize: 12
        }

        RowLayout {
            width: parent.width
            spacing: 6

            DimensionInput {
                Layout.fillWidth: true
                theme: root.theme
                label: qsTr("X")
                value: root.coordX
                decimals: root.decimals
                minValue: -1e9
                accentColor: root.theme.axisX

                onValueEdited: function(newVal) {
                    root.coordX = newVal;
                    root.coordinateChanged(root.coordX, root.coordY, root.coordZ);
                }
            }

            DimensionInput {
                Layout.fillWidth: true
                theme: root.theme
                label: qsTr("Y")
                value: root.coordY
                decimals: root.decimals
                minValue: -1e9
                accentColor: root.theme.axisY

                onValueEdited: function(newVal) {
                    root.coordY = newVal;
                    root.coordinateChanged(root.coordX, root.coordY, root.coordZ);
                }
            }

            DimensionInput {
                Layout.fillWidth: true
                theme: root.theme
                label: qsTr("Z")
                value: root.coordZ
                decimals: root.decimals
                minValue: -1e9
                accentColor: root.theme.axisZ

                onValueEdited: function(newVal) {
                    root.coordZ = newVal;
                    root.coordinateChanged(root.coordX, root.coordY, root.coordZ);
                }
            }
        }
    }
}
