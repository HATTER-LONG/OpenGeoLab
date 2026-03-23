pragma ComponentBehavior: Bound

import QtQuick

import "../theme"
import "." as Components

Rectangle {
    id: pluginGroup

    required property AppTheme theme
    required property var actionHandler
    property var plugins: []

    readonly property var pluginIcons: [
        "pluginA", "pluginB", "pluginC", "pluginD", "pluginE",
        "pluginF", "pluginG", "pluginH", "pluginI", "pluginJ"
    ]

    color: "transparent"
    implicitWidth: pluginGroup.plugins.length > 0 ? pluginRow.implicitWidth + 16 : emptyLabel.implicitWidth + 32
    implicitHeight: parent ? parent.height : 80

    Row {
        id: pluginRow

        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4
        visible: pluginGroup.plugins.length > 0

        Repeater {
            model: pluginGroup.plugins

            delegate: Components.RibbonTile {
                required property int index
                required property var modelData

                width: 72
                height: 72
                theme: pluginGroup.theme
                title: modelData.name ?? qsTr("Plugin")
                iconKind: pluginGroup.pluginIcons[index % pluginGroup.pluginIcons.length]
                accentOne: pluginGroup.theme.accentE
                accentTwo: pluginGroup.theme.accentA
                actionKey: modelData.hasUI ? "pluginUI_" + modelData.moduleName : "plugin_" + modelData.moduleName
                actionHandler: pluginGroup.actionHandler
            }
        }
    }

    Text {
        id: emptyLabel

        anchors.centerIn: parent
        visible: pluginGroup.plugins.length === 0
        text: qsTr("No plugins found")
        color: pluginGroup.theme.textTertiary
        font.pixelSize: 12
    }
}
