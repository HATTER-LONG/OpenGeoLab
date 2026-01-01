pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "." as Pages

/**
 * @brief Dialog for trimming geometry.
 */
Pages.BaseDialog {
    id: root

    title: qsTr("Trim")
    okButtonText: qsTr("Apply")

    property var initialParams: ({})

    okEnabled: !OGL.BackendService.busy

    onAccepted: {
        const params = {
            mode: modeCombo.currentText,
            keepOriginal: keepOriginalCheck.checked
        };
        OGL.BackendService.request("trim", params);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Label {
            Layout.fillWidth: true
            text: qsTr("Select geometry to trim and configure options:")
            color: Theme.textSecondaryColor
            wrapMode: Text.Wrap
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 12
            rowSpacing: 12

            Label {
                text: qsTr("Trim Mode:")
                color: Theme.textPrimaryColor
            }
            ComboBox {
                id: modeCombo
                Layout.fillWidth: true
                model: [qsTr("Auto"), qsTr("By Plane"), qsTr("By Surface")]
                enabled: !OGL.BackendService.busy
            }

            Label {
                text: qsTr("Options:")
                color: Theme.textPrimaryColor
            }
            CheckBox {
                id: keepOriginalCheck
                text: qsTr("Keep original geometry")
                checked: false
                enabled: !OGL.BackendService.busy
            }
        }

        // Placeholder for geometry selection UI.
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 60
            color: Theme.surfaceAltColor
            radius: 6
            border.width: 1
            border.color: Theme.borderColor

            Label {
                anchors.centerIn: parent
                text: qsTr("Click to select geometry...")
                color: Theme.textSecondaryColor
                font.italic: true
            }
        }

        // Status area.
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: OGL.BackendService.busy

            BusyIndicator {
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
                running: true
            }

            Label {
                Layout.fillWidth: true
                text: OGL.BackendService.message
                color: Theme.textSecondaryColor
                elide: Text.ElideRight
            }
        }
    }

    Connections {
        target: OGL.BackendService

        function onOperationFinished(moduleName: string, _result: var): void {
            if (moduleName === "Trim")
                root.closeRequested();
        }
    }
}
