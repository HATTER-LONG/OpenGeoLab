pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "." as Pages

/**
 * @brief Dialog for offsetting geometry.
 */
Pages.BaseDialog {
    id: root

    title: qsTr("Offset")
    okButtonText: qsTr("Apply")

    property var initialParams: ({})

    okEnabled: !OGL.BackendService.busy && !isNaN(parseFloat(distanceInput.text))

    onAccepted: {
        const params = {
            distance: parseFloat(distanceInput.text) || 0,
            direction: directionCombo.currentText,
            keepOriginal: keepOriginalCheck.checked
        };
        OGL.BackendService.request("offset", params);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Label {
            Layout.fillWidth: true
            text: qsTr("Configure offset parameters:")
            color: Theme.textSecondaryColor
            wrapMode: Text.Wrap
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 12
            rowSpacing: 12

            Label {
                text: qsTr("Distance:")
                color: Theme.textPrimaryColor
            }
            TextField {
                id: distanceInput
                Layout.fillWidth: true
                placeholderText: "1.0"
                text: "1.0"
                validator: DoubleValidator {}
                enabled: !OGL.BackendService.busy
            }

            Label {
                text: qsTr("Direction:")
                color: Theme.textPrimaryColor
            }
            ComboBox {
                id: directionCombo
                Layout.fillWidth: true
                model: [qsTr("Normal"), qsTr("Inward"), qsTr("Outward")]
                enabled: !OGL.BackendService.busy
            }

            Label {
                text: qsTr("Options:")
                color: Theme.textPrimaryColor
            }
            CheckBox {
                id: keepOriginalCheck
                text: qsTr("Keep original geometry")
                checked: true
                enabled: !OGL.BackendService.busy
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
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
            if (moduleName === "Offset")
                root.closeRequested();
        }
    }
}
