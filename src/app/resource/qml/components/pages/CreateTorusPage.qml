import QtQuick
import QtQuick.Layouts
import "../.."
import ".."

FunctionPageBase {
    id: root

    pageTitle: qsTr("Create Torus")
    pageIcon: "torus"
    actionId: "addTorus"

    property string torusName: ""
    property real centerX: 0.0
    property real centerY: 0.0
    property real centerZ: 0.0
    property real majorRadius: 10.0
    property real minorRadius: 3.0

    function getParameters() {
        return {
            module: "geometry",
            action: "create_torus",
            param: {
                name: root.torusName,
                origin: [root.centerX, root.centerY, root.centerZ],
                majorRadius: root.majorRadius, minorRadius: root.minorRadius
            },
            mute: false
        };
    }

    ParamField {
        width: parent.width
        theme: root.theme
        label: qsTr("Torus Name")
        placeholder: qsTr("Auto-generated if empty")
        value: root.torusName

        onValueEdited: function(newValue) {
            root.torusName = newValue;
        }
    }

    CoordinateField {
        width: parent.width
        theme: root.theme
        label: qsTr("Center Point")
        coordX: root.centerX
        coordY: root.centerY
        coordZ: root.centerZ

        onCoordinateChanged: function(x, y, z) {
            root.centerX = x;
            root.centerY = y;
            root.centerZ = z;
        }
    }

    Text {
        text: qsTr("Dimensions")
        color: root.theme.textSecondary
        font.pixelSize: 12
    }

    RowLayout {
        width: parent.width
        spacing: 6

        DimensionInput {
            Layout.fillWidth: true
            theme: root.theme
            label: qsTr("R1")
            value: root.majorRadius
            accentColor: root.theme.axisX
            tooltipText: qsTr("Major Radius")

            onValueEdited: function(newVal) {
                root.majorRadius = newVal;
            }
        }

        DimensionInput {
            Layout.fillWidth: true
            theme: root.theme
            label: qsTr("R2")
            value: root.minorRadius
            accentColor: root.theme.axisY
            tooltipText: qsTr("Minor Radius")

            onValueEdited: function(newVal) {
                root.minorRadius = newVal;
            }
        }
    }

    Rectangle {
        width: parent.width
        height: warningText.implicitHeight + 12
        radius: root.theme.radiusSmall
        color: Qt.rgba(root.theme.accentC.r, root.theme.accentC.g, root.theme.accentC.b, 0.15)
        border.width: 1
        border.color: Qt.rgba(root.theme.accentC.r, root.theme.accentC.g, root.theme.accentC.b, 0.4)
        visible: root.minorRadius >= root.majorRadius

        Text {
            id: warningText
            anchors.centerIn: parent
            width: parent.width - 16
            text: qsTr("Minor radius should be less than major radius")
            color: root.theme.accentC
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
    }

    Rectangle {
        width: parent.width
        height: infoColumn.implicitHeight + 16
        radius: root.theme.radiusSmall
        color: root.theme.surfaceMuted

        Column {
            id: infoColumn

            anchors.fill: parent
            anchors.margins: 8
            spacing: 4

            RowLayout {
                width: parent.width
                spacing: 4

                Text {
                    Layout.preferredWidth: 100
                    text: qsTr("Volume:")
                    color: root.theme.textSecondary
                    font.pixelSize: 11
                }

                Text {
                    Layout.fillWidth: true
                    text: (2 * Math.PI * Math.PI * root.majorRadius * root.minorRadius * root.minorRadius).toFixed(3)
                    color: root.theme.textPrimary
                    font.pixelSize: 11
                    font.bold: true
                    horizontalAlignment: Text.AlignLeft
                }
            }

            RowLayout {
                width: parent.width
                spacing: 4

                Text {
                    Layout.preferredWidth: 100
                    text: qsTr("Surface Area:")
                    color: root.theme.textSecondary
                    font.pixelSize: 11
                }

                Text {
                    Layout.fillWidth: true
                    text: (4 * Math.PI * Math.PI * root.majorRadius * root.minorRadius).toFixed(3)
                    color: root.theme.textPrimary
                    font.pixelSize: 11
                    font.bold: true
                    horizontalAlignment: Text.AlignLeft
                }
            }

            RowLayout {
                width: parent.width
                spacing: 4

                Text {
                    Layout.preferredWidth: 100
                    text: qsTr("Outer Diameter:")
                    color: root.theme.textSecondary
                    font.pixelSize: 11
                }

                Text {
                    Layout.fillWidth: true
                    text: (2 * (root.majorRadius + root.minorRadius)).toFixed(3)
                    color: root.theme.textPrimary
                    font.pixelSize: 11
                    font.bold: true
                    horizontalAlignment: Text.AlignLeft
                }
            }
        }
    }
}
