/**
 * @file AddLinePage.qml
 * @brief Function page for creating a line
 *
 * Allows user to input start and end points to create a line geometry.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

FunctionPageBase {
    id: root

    pageTitle: qsTr("Create Line")
    pageIcon: "qrc:/opengeolab/resources/icons/line.svg"
    serviceName: "GeometryService"
    actionId: "addLine"

    width: 340

    // =========================================================
    // Parameters
    // =========================================================

    /// Start point coordinates
    property real startX: 0.0
    property real startY: 0.0
    property real startZ: 0.0

    /// End point coordinates
    property real endX: 10.0
    property real endY: 0.0
    property real endZ: 0.0

    /// Line name
    property string lineName: ""

    function getParameters(): var {
        return {
            "action": "createLine",
            "name": lineName || "Line_" + Date.now(),
            "start": {
                "x": startX,
                "y": startY,
                "z": startZ
            },
            "end": {
                "x": endX,
                "y": endY,
                "z": endZ
            }
        };
    }

    // =========================================================
    // Content
    // =========================================================

    Column {
        width: parent.width
        spacing: 12

        ParamField {
            label: qsTr("Line Name")
            placeholder: qsTr("Auto-generated if empty")
            value: root.lineName
            onValueEdited: newValue => root.lineName = newValue
        }

        CoordinateField {
            label: qsTr("Start Point")
            coordX: root.startX
            coordY: root.startY
            coordZ: root.startZ
            onCoordinateChanged: (x, y, z) => {
                root.startX = x;
                root.startY = y;
                root.startZ = z;
            }
        }

        CoordinateField {
            label: qsTr("End Point")
            coordX: root.endX
            coordY: root.endY
            coordZ: root.endZ
            onCoordinateChanged: (x, y, z) => {
                root.endX = x;
                root.endY = y;
                root.endZ = z;
            }
        }

        // Preview info
        Rectangle {
            width: parent.width
            height: lengthRow.implicitHeight + 12
            radius: 4
            color: Theme.surfaceAlt

            RowLayout {
                id: lengthRow
                anchors.fill: parent
                anchors.margins: 6
                spacing: 8

                Label {
                    text: qsTr("Length:")
                    font.pixelSize: 11
                    color: Theme.textSecondary
                }

                Label {
                    property real dx: root.endX - root.startX
                    property real dy: root.endY - root.startY
                    property real dz: root.endZ - root.startZ
                    property real length: Math.sqrt(dx * dx + dy * dy + dz * dz)

                    text: length.toFixed(3)
                    font.pixelSize: 11
                    font.bold: true
                    color: Theme.textPrimary
                }
            }
        }
    }
}
