import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Read-only panel displaying the action's JSON schema.
 * The Python backend attaches a JsonHighlighter to this
 * component's TextArea document via objectName lookup.
 */
Item {
    id: root

    property string json: ""
    property string placeholderText: qsTr("← Select an action from the tree.")

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        Label {
            text: qsTr("Schema")
            color: PluginTheme.textSecondary
            font.pixelSize: 12
            font.weight: Font.DemiBold
            Layout.leftMargin: PluginTheme.gapTight
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: PluginTheme.surfaceMuted
            radius: PluginTheme.radiusSmall

            ScrollView {
                anchors.fill: parent
                anchors.margins: PluginTheme.gapTight

                TextArea {
                    objectName: "detailTextArea"
                    text: root.json || root.placeholderText
                    readOnly: true
                    wrapMode: TextEdit.Wrap
                    selectByMouse: true
                    font.family: PluginTheme.monoFont
                    font.pixelSize: 13
                    color: root.json
                           ? PluginTheme.textPrimary
                           : PluginTheme.textTertiary
                    selectionColor: PluginTheme.accentA
                    selectedTextColor: PluginTheme.textOnAccent

                    background: Rectangle {
                        color: "transparent"
                    }
                }
            }
        }
    }
}
