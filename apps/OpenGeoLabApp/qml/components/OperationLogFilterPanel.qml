pragma ComponentBehavior: Bound

import QtQuick

import "../theme"

Rectangle {
    id: root

    required property AppTheme theme
    required property var logService
    property var levelOptions: [
        { "level": 0, "label": qsTr("Trace") },
        { "level": 1, "label": qsTr("Debug") },
        { "level": 2, "label": qsTr("Info") },
        { "level": 3, "label": qsTr("Warn") },
        { "level": 4, "label": qsTr("Error") },
        { "level": 5, "label": qsTr("Critical") }
    ]
    readonly property int runtimeMinLevel: root.logService ? root.logService.minLevel : 2
    readonly property int enabledLevelMask: root.logService ? root.logService.enabledLevelMask : 0x3F

    function levelTint(level) {
        if (level >= 4) {
            return root.theme.accentD;
        }
        if (level === 3) {
            return root.theme.accentC;
        }
        if (level === 2) {
            return root.theme.accentB;
        }
        return root.theme.accentA;
    }

    function levelVisible(level) {
        return ((root.enabledLevelMask >> level) & 1) !== 0;
    }

    radius: root.theme.radiusSmall
    color: root.theme.tint(root.theme.surfaceMuted, root.theme.darkMode ? 0.84 : 0.97)
    border.width: 1
    border.color: root.theme.tint(root.theme.borderSubtle, 0.8)
    implicitHeight: filterColumn.implicitHeight + 16

    Column {
        id: filterColumn

        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        Text {
            text: qsTr("spdlog output level")
            color: root.theme.textSecondary
            font.pixelSize: 11
            font.bold: true
            font.family: root.theme.bodyFontFamily
        }

        Text {
            width: parent.width
            text: qsTr("This controls which new log messages are emitted by the runtime logger pipeline.")
            wrapMode: Text.WordWrap
            color: root.theme.textTertiary
            font.pixelSize: 10
            font.family: root.theme.bodyFontFamily
        }

        Flow {
            width: parent.width
            spacing: 8

            Repeater {
                model: root.levelOptions

                delegate: OperationLogLevelChip {
                    required property var modelData

                    theme: root.theme
                    text: modelData.label
                    accentColor: root.levelTint(modelData.level)
                    selected: root.runtimeMinLevel === modelData.level
                    onClicked: {
                        if (root.logService) {
                            root.logService.setMinLevel(modelData.level);
                        }
                    }
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: root.theme.tint(root.theme.borderSubtle, 0.82)
        }

        Text {
            text: qsTr("Visible log entries")
            color: root.theme.textSecondary
            font.pixelSize: 11
            font.bold: true
            font.family: root.theme.bodyFontFamily
        }

        Text {
            width: parent.width
            text: qsTr("This only filters the entries already captured inside the panel.")
            wrapMode: Text.WordWrap
            color: root.theme.textTertiary
            font.pixelSize: 10
            font.family: root.theme.bodyFontFamily
        }

        Flow {
            width: parent.width
            spacing: 8

            Repeater {
                model: root.levelOptions

                delegate: OperationLogLevelChip {
                    required property var modelData

                    readonly property bool selectedLevel: root.levelVisible(modelData.level)

                    theme: root.theme
                    text: modelData.label
                    accentColor: root.levelTint(modelData.level)
                    selected: selectedLevel
                    onClicked: {
                        if (root.logService) {
                            root.logService.setLevelEnabled(modelData.level, !selectedLevel);
                        }
                    }
                }
            }
        }
    }
}
