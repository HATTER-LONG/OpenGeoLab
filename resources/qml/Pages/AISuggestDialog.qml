pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "." as Pages

/**
 * @brief Dialog for AI suggestions on geometry operations.
 */
Pages.BaseDialog {
    id: root

    title: qsTr("AI Suggest")
    okButtonText: qsTr("Get Suggestions")

    property var initialParams: ({})

    okEnabled: !OGL.BackendService.busy && promptInput.text.trim().length > 0

    onAccepted: {
        const params = {
            prompt: promptInput.text.trim(),
            context: contextCombo.currentText
        };
        OGL.BackendService.request("aiSuggest", params);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Label {
            Layout.fillWidth: true
            text: qsTr("Describe what you want to achieve:")
            color: Theme.textSecondaryColor
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Label {
                text: qsTr("Context:")
                color: Theme.textPrimaryColor
            }
            ComboBox {
                id: contextCombo
                Layout.fillWidth: true
                model: [qsTr("Geometry"), qsTr("Mesh"), qsTr("All")]
                enabled: !OGL.BackendService.busy
            }
        }

        TextArea {
            id: promptInput
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 80
            placeholderText: qsTr("e.g., Create a cylinder with radius 5 and height 10...")
            wrapMode: TextArea.Wrap
            enabled: !OGL.BackendService.busy

            background: Rectangle {
                color: Theme.surfaceAltColor
                radius: 6
                border.width: 1
                border.color: promptInput.activeFocus ? Theme.primaryColor : Theme.borderColor
            }
        }

        // Results area (placeholder).
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: Theme.surfaceAltColor
            radius: 6
            border.width: 1
            border.color: Theme.borderColor
            visible: !OGL.BackendService.busy

            Label {
                anchors.centerIn: parent
                text: qsTr("Suggestions will appear here...")
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

        function onOperationFinished(actionId, _result) {
            if (actionId === "aiSuggest") {
                // Keep dialog open to show results.
            }
        }
    }
}
