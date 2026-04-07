import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import OpenGeoLab.Services 1.0
import "../.."
import ".."

FunctionPageBase {
    id: root

    pageTitle: qsTr("Import Model")
    pageIcon: "import"
    actionId: "importModel"

    property string selectedFilePath: ""
    property string selectedFormat: ""
    property string shapeName: ""
    property bool keepTriangulation: false

    function getParameters() {
        const action = root.selectedFormat === "brep" ? "import_brep" : "import_step";
        let params = {
            module: "geometry",
            action: action,
            param: {
                path: root.selectedFilePath,
                name: root.shapeName,
                tessellate: true
            },
            mute: false
        };
        if (root.selectedFormat === "brep" && root.keepTriangulation) {
            params.param.keepTriangulation = true;
        }
        return params;
    }

    function open(payload) {
        root.selectedFilePath = "";
        root.selectedFormat = "";
        root.shapeName = "";
        root.keepTriangulation = false;
        root.x = 292;
        root.y = 0;
        root.pageVisible = true;
        root.forceActiveFocus();
    }

    function execute() {
        if (RequestService.busy) return;
        if (root.selectedFilePath.length === 0) {
            return;
        }
        RequestService.submitAsync(JSON.stringify(root.getParameters()));
        root.close();
    }

    FileDialog {
        id: fileDialog

        title: qsTr("Select Model File")
        nameFilters: [
            qsTr("All Supported Formats") + " (*.brep *.brp *.step *.stp)",
            qsTr("BRep Files") + " (*.brep *.brp)",
            qsTr("STEP Files") + " (*.step *.stp)"
        ]

        onAccepted: {
            let path = selectedFile.toString();
            if (path.startsWith("file:///")) {
                path = path.substring(8);
            }
            root.selectedFilePath = path;

            const lower = path.toLowerCase();
            if (lower.endsWith(".brep") || lower.endsWith(".brp")) {
                root.selectedFormat = "brep";
            } else if (lower.endsWith(".step") || lower.endsWith(".stp")) {
                root.selectedFormat = "step";
            }

            if (root.shapeName.length === 0) {
                const parts = path.replace(/\\/g, "/").split("/");
                const fileName = parts[parts.length - 1];
                const dotIndex = fileName.lastIndexOf(".");
                root.shapeName = dotIndex > 0 ? fileName.substring(0, dotIndex) : fileName;
            }
        }
    }

    Rectangle {
        width: parent.width
        height: browseRow.implicitHeight + 16
        radius: root.theme.radiusSmall
        color: root.theme.surfaceMuted
        border.width: 1
        border.color: root.selectedFilePath.length > 0
            ? root.theme.tint(root.theme.accentA, 0.3)
            : root.theme.borderSubtle

        RowLayout {
            id: browseRow

            anchors.fill: parent
            anchors.margins: 8
            spacing: 8

            Text {
                Layout.fillWidth: true
                text: root.selectedFilePath.length > 0
                    ? root.selectedFilePath
                    : qsTr("No file selected")
                color: root.selectedFilePath.length > 0
                    ? root.theme.textPrimary
                    : root.theme.textSecondary
                font.pixelSize: 12
                elide: Text.ElideMiddle
                verticalAlignment: Text.AlignVCenter
            }

            Rectangle {
                Layout.preferredWidth: 72
                Layout.preferredHeight: 26
                radius: root.theme.radiusSmall
                color: browseMouseArea.containsMouse
                    ? root.theme.tint(root.theme.accentA, root.theme.darkMode ? 0.22 : 0.14)
                    : root.theme.surface
                border.width: 1
                border.color: browseMouseArea.containsMouse
                    ? root.theme.accentA
                    : root.theme.borderSubtle

                Text {
                    anchors.centerIn: parent
                    text: qsTr("Browse")
                    color: browseMouseArea.containsMouse
                        ? root.theme.accentA
                        : root.theme.textPrimary
                    font.pixelSize: 12
                    font.bold: true
                }

                MouseArea {
                    id: browseMouseArea

                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: fileDialog.open()
                }
            }
        }
    }

    ParamField {
        width: parent.width
        theme: root.theme
        label: qsTr("Shape Name")
        placeholder: qsTr("Auto-generated from filename")
        value: root.shapeName

        onValueEdited: function(newValue) {
            root.shapeName = newValue;
        }
    }

    Rectangle {
        width: parent.width
        height: formatColumn.implicitHeight + 16
        radius: root.theme.radiusSmall
        color: root.theme.surfaceMuted
        visible: root.selectedFormat.length > 0

        Column {
            id: formatColumn

            anchors.fill: parent
            anchors.margins: 8
            spacing: 4

            RowLayout {
                width: parent.width
                spacing: 4

                Text {
                    Layout.preferredWidth: 56
                    text: qsTr("Format:")
                    color: root.theme.textSecondary
                    font.pixelSize: 11
                }

                Text {
                    Layout.fillWidth: true
                    text: root.selectedFormat === "brep"
                        ? "BRep (OpenCASCADE)"
                        : root.selectedFormat === "step"
                            ? "STEP (ISO 10303)"
                            : qsTr("Unknown")
                    color: root.theme.textPrimary
                    font.pixelSize: 11
                    font.bold: true
                }
            }
        }
    }

    Rectangle {
        width: parent.width
        height: keepTriRow.implicitHeight + 16
        radius: root.theme.radiusSmall
        color: root.theme.surfaceMuted
        visible: root.selectedFormat === "brep"

        RowLayout {
            id: keepTriRow

            anchors.fill: parent
            anchors.margins: 8
            spacing: 8

            Rectangle {
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                radius: 3
                color: root.keepTriangulation
                    ? root.theme.accentA
                    : "transparent"
                border.width: 1
                border.color: root.keepTriangulation
                    ? root.theme.accentA
                    : root.theme.borderSubtle

                Text {
                    anchors.centerIn: parent
                    text: "✓"
                    color: root.theme.textOnAccent
                    font.pixelSize: 11
                    font.bold: true
                    visible: root.keepTriangulation
                }
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Keep existing triangulation")
                color: root.theme.textPrimary
                font.pixelSize: 12
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: root.keepTriangulation = !root.keepTriangulation
        }
    }

    Text {
        width: parent.width
        visible: root.selectedFilePath.length === 0
        text: qsTr("Select a BRep (.brep, .brp) or STEP (.step, .stp) file to import.")
        color: root.theme.textSecondary
        font.pixelSize: 11
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
    }
}
