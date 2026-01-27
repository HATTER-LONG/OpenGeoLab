/**
 * @file AISuggestPage.qml
 * @brief Function page for AI suggestions
 *
 * Allows user to get AI-powered suggestions for geometry operations.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../util"
import OpenGeoLab 1.0

FunctionPageBase {
    id: root

    pageTitle: qsTr("AI Suggest")
    pageIcon: "qrc:/opengeolab/resources/icons/ai_suggestion.svg"
    serviceName: "AIService"
    actionId: "aiSuggest"

    width: 360

    // =========================================================
    // Parameters
    // =========================================================

    /// User query/prompt
    property string userPrompt: ""
    /// Selected context geometries
    property var contextGeometries: []
    /// Suggestion mode
    property string mode: "auto"

    function getParameters() {
        return {
            "action": "suggest",
            "prompt": userPrompt,
            "context": contextGeometries,
            "mode": mode
        };
    }

    // =========================================================
    // Content
    // =========================================================

    Column {
        width: parent.width
        spacing: 12

        // Context info
        Rectangle {
            width: parent.width
            height: contextRow.implicitHeight + 12
            radius: 4
            color: Theme.surfaceAlt
            border.width: 1
            border.color: Theme.border

            RowLayout {
                id: contextRow
                anchors.fill: parent
                anchors.margins: 6
                spacing: 8

                Label {
                    text: "🔮"
                    font.pixelSize: 16
                }

                Label {
                    text: root.contextGeometries.length > 0 ? qsTr("%1 objects in context").arg(root.contextGeometries.length) : qsTr("Analyzing workspace...")
                    font.pixelSize: 11
                    color: Theme.textPrimary
                    Layout.fillWidth: true
                }
            }
        }

        // Mode selection
        Column {
            width: parent.width
            spacing: 4

            Label {
                text: qsTr("Suggestion Mode")
                font.pixelSize: 11
                color: Theme.textSecondary
            }

            RowLayout {
                width: parent.width
                spacing: 6

                ModeButton {
                    Layout.fillWidth: true
                    text: qsTr("Auto")
                    description: qsTr("AI decides")
                    selected: root.mode === "auto"
                    onClicked: root.mode = "auto"
                }

                ModeButton {
                    Layout.fillWidth: true
                    text: qsTr("Optimize")
                    description: qsTr("Improve mesh")
                    selected: root.mode === "optimize"
                    onClicked: root.mode = "optimize"
                }

                ModeButton {
                    Layout.fillWidth: true
                    text: qsTr("Repair")
                    description: qsTr("Fix issues")
                    selected: root.mode === "repair"
                    onClicked: root.mode = "repair"
                }
            }
        }

        // Prompt input
        Column {
            width: parent.width
            spacing: 4

            Label {
                text: qsTr("Describe what you want to achieve")
                font.pixelSize: 11
                color: Theme.textSecondary
            }

            Rectangle {
                width: parent.width
                height: 80
                radius: 4
                color: Theme.surface
                border.width: promptArea.activeFocus ? 2 : 1
                border.color: promptArea.activeFocus ? Theme.accent : Theme.border

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 8

                    TextArea {
                        id: promptArea
                        text: root.userPrompt
                        placeholderText: qsTr("E.g., 'Optimize the mesh quality while preserving sharp edges'")
                        wrapMode: TextEdit.Wrap
                        font.pixelSize: 12
                        color: Theme.textPrimary
                        placeholderTextColor: Theme.textDisabled

                        background: Item {}

                        onTextChanged: root.userPrompt = text
                    }
                }
            }

            Label {
                text: qsTr("Optional: Leave empty for automatic suggestions")
                font.pixelSize: 10
                color: Theme.textDisabled
                font.italic: true
            }
        }

        // Quick prompts
        Column {
            width: parent.width
            spacing: 4

            Label {
                text: qsTr("Quick Prompts")
                font.pixelSize: 11
                color: Theme.textSecondary
            }

            Flow {
                width: parent.width
                spacing: 6

                ChipButton {
                    text: qsTr("Optimize mesh")
                    onClicked: root.userPrompt = "Optimize mesh quality and element distribution"
                }

                ChipButton {
                    text: qsTr("Fix normals")
                    onClicked: root.userPrompt = "Fix inconsistent normals and repair surface orientation"
                }

                ChipButton {
                    text: qsTr("Simplify")
                    onClicked: root.userPrompt = "Simplify geometry while preserving key features"
                }

                ChipButton {
                    text: qsTr("Fill holes")
                    onClicked: root.userPrompt = "Detect and fill holes in the mesh"
                }
            }
        }
    }

    // =========================================================
    // Mode button component
    // =========================================================
    component ModeButton: SelectableButton {
        id: modeBtn

        property string description: ""

        implicitHeight: 48

        contentItem: Column {
            spacing: 2

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: modeBtn.text
                font.pixelSize: 11
                font.bold: modeBtn.selected
                color: modeBtn.selected ? Theme.white : Theme.textPrimary
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: modeBtn.description
                font.pixelSize: 9
                color: modeBtn.selected ? Qt.rgba(1, 1, 1, 0.7) : Theme.textSecondary
            }
        }
    }
}
