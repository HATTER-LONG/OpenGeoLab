/**
 * @file AddPointPage.qml
 * @brief Function page for creating a point
 *
 * Allows user to input 3D coordinates to create a point geometry.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

FunctionPageBase {
    id: root

    pageTitle: qsTr("Create Point")
    pageIcon: "qrc:/opengeolab/resources/icons/point.svg"
    serviceName: "GeometryService"
    actionId: "addPoint"

    // =========================================================
    // Parameters
    // =========================================================

    /// Point coordinates
    property real pointX: 0.0
    property real pointY: 0.0
    property real pointZ: 0.0

    /// Point name
    property string pointName: ""

    function getParameters(): var {
        return {
            "action": "createPoint",
            "name": pointName || "Point_" + Date.now(),
            "coordinates": {
                "x": pointX,
                "y": pointY,
                "z": pointZ
            }
        };
    }

    function parsePayload(payload: var): void {
        if (payload.x !== undefined)
            root.pointX = payload.x;
        if (payload.y !== undefined)
            root.pointY = payload.y;
        if (payload.z !== undefined)
            root.pointZ = payload.z;
    }

    // =========================================================
    // Content
    // =========================================================

    Column {
        width: parent.width
        spacing: 12

        ParamField {
            label: qsTr("Point Name")
            placeholder: qsTr("Auto-generated if empty")
            value: root.pointName
            onValueEdited: newValue => root.pointName = newValue
        }

        CoordinateField {
            label: qsTr("Coordinates")
            coordX: root.pointX
            coordY: root.pointY
            coordZ: root.pointZ
            onCoordinateChanged: (x, y, z) => {
                root.pointX = x;
                root.pointY = y;
                root.pointZ = z;
            }
        }

        // Hint section
        Rectangle {
            width: parent.width
            height: hintText.implicitHeight + 12
            radius: 4
            color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.1)
            border.width: 1
            border.color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.3)

            Label {
                id: hintText
                anchors.fill: parent
                anchors.margins: 6
                text: qsTr("💡 Tip: You can also click in the viewport to pick coordinates.")
                font.pixelSize: 11
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
            }
        }
    }
}
