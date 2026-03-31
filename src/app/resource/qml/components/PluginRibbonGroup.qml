pragma ComponentBehavior: Bound

import QtQuick

import "../theme"
import "." as Components

Item {
    id: pluginGroup

    required property AppTheme theme
    required property var actionHandler
    property var plugins: []

    readonly property var pluginIcons: ["pluginA", "pluginB", "pluginC", "pluginD", "pluginE", "pluginF", "pluginG", "pluginH", "pluginI", "pluginJ"]

    implicitWidth: pluginGroup.plugins.length > 0 ? pluginRow.implicitWidth + 20 : emptyLabel.implicitWidth + 32
    implicitHeight: parent ? parent.height : 80

    Item {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: titleLabel.top
        anchors.leftMargin: 6
        anchors.rightMargin: 8
        anchors.topMargin: 4
        anchors.bottomMargin: 8

        Row {
            id: pluginRow

            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 6
            visible: pluginGroup.plugins.length > 0

            Repeater {
                model: pluginGroup.plugins

                delegate: Components.RibbonTile {
                    required property int index
                    required property var modelData

                    width: 68
                    height: 68
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

    Text {
        id: titleLabel

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 1
        text: qsTr("Plugin")
        color: pluginGroup.theme.textTertiary
        font.pixelSize: 9
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
    }
}
