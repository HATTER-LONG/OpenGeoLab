import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Horizontal split view showing request JSON (editable, left)
 * and response JSON (read-only, right).
 *
 * Python JsonHighlighter is attached to TextArea documents
 * via objectName lookup after QML load.
 */
Item {
    id: root

    property string requestJson: ""
    property string responseJson: ""
    signal requestEdited(string text)

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        // ── Request panel ──────────────────────────────────────────────
        Item {
            SplitView.fillWidth: true
            SplitView.minimumWidth: 200

            ColumnLayout {
                anchors.fill: parent
                spacing: 4

                Label {
                    text: qsTr("Request")
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
                            objectName: "requestTextArea"
                            text: root.requestJson
                            wrapMode: TextEdit.Wrap
                            selectByMouse: true
                            font.family: PluginTheme.monoFont
                            font.pixelSize: 13
                            color: PluginTheme.textPrimary
                            selectionColor: PluginTheme.accentA
                            selectedTextColor: PluginTheme.textOnAccent
                            placeholderText: '{"module": "…", "action": "…", "param": {}}'

                            background: Rectangle { color: "transparent" }

                            onTextChanged: {
                                if (activeFocus && text !== root.requestJson) {
                                    root.requestEdited(text);
                                }
                            }
                        }
                    }
                }
            }
        }

        // ── Response panel ─────────────────────────────────────────────
        Item {
            SplitView.fillWidth: true
            SplitView.minimumWidth: 200

            ColumnLayout {
                anchors.fill: parent
                spacing: 4

                Label {
                    text: qsTr("Response")
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
                            objectName: "responseTextArea"
                            text: root.responseJson
                            readOnly: true
                            wrapMode: TextEdit.Wrap
                            selectByMouse: true
                            font.family: PluginTheme.monoFont
                            font.pixelSize: 13
                            color: root.responseJson
                                   ? PluginTheme.textPrimary
                                   : PluginTheme.textTertiary
                            selectionColor: PluginTheme.accentA
                            selectedTextColor: PluginTheme.textOnAccent
                            placeholderText: qsTr("Execute an action to see results.")

                            background: Rectangle { color: "transparent" }
                        }
                    }
                }
            }
        }
    }
}
