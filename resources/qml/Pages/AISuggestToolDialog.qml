pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "." as Pages

/**
 * @file AISuggestToolDialog.qml
 * @brief Non-modal tool dialog for AI geometry suggestions
 *
 * Allows users to describe operations and receive AI-generated suggestions.
 */
Pages.ToolDialog {
    id: root

    title: qsTr("AI Suggest")
    okButtonText: qsTr("Get Suggestions")
    preferredContentHeight: 340

    okEnabled: !OGL.BackendService.busy && promptInput.text.trim().length > 0

    onAccepted: {
        const params = {
            prompt: promptInput.text.trim(),
            context: contextCombo.currentText
        };
        OGL.BackendService.request("aiSuggest", params);
    }

    ColumnLayout {
        width: parent.width
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: qsTr("Describe what you want to achieve:")
            color: Theme.textSecondaryColor
            wrapMode: Text.Wrap
            font.pixelSize: 12
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

                contentItem: Text {
                    text: contextCombo.displayText
                    color: Theme.textPrimaryColor
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    leftPadding: 10
                }

                background: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 32
                    color: contextCombo.enabled ? Theme.surfaceColor : Theme.surfaceAltColor
                    border.width: 1
                    border.color: contextCombo.pressed ? Theme.accentColor : Theme.borderColor
                    radius: 4
                }
            }
        }

        TextArea {
            id: promptInput
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            placeholderText: qsTr("e.g., Create a cylinder with radius 5 and height 10...")
            wrapMode: TextArea.Wrap
            enabled: !OGL.BackendService.busy

            color: Theme.textPrimaryColor
            placeholderTextColor: Theme.textSecondaryColor

            background: Rectangle {
                color: promptInput.enabled ? Theme.surfaceColor : Theme.surfaceAltColor
                radius: 6
                border.width: 1
                border.color: promptInput.activeFocus ? Theme.primaryColor : Theme.borderColor
            }
        }

        // Results area
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            color: Theme.surfaceAltColor
            radius: 6
            border.width: 1
            border.color: Theme.borderColor

            ScrollView {
                anchors.fill: parent
                anchors.margins: 8
                clip: true

                Label {
                    id: resultLabel
                    width: parent.width
                    text: resultLabel.resultText || qsTr("Suggestions will appear here...")
                    color: resultLabel.resultText ? Theme.textPrimaryColor : Theme.textSecondaryColor
                    font.italic: !resultLabel.resultText
                    wrapMode: Text.Wrap

                    property string resultText: ""
                }
            }
        }

        // Status area
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: OGL.BackendService.busy

            BusyIndicator {
                Layout.preferredWidth: 20
                Layout.preferredHeight: 20
                running: true
            }

            Label {
                Layout.fillWidth: true
                text: OGL.BackendService.message
                color: Theme.textSecondaryColor
                font.pixelSize: 12
                elide: Text.ElideRight
            }
        }
    }

    Connections {
        target: OGL.BackendService

        function onOperationFinished(moduleName: string, result: var): void {
            if (moduleName === "AISuggest") {
                resultLabel.resultText = result.suggestion || qsTr("No suggestions available.");
            }
        }
    }
}
