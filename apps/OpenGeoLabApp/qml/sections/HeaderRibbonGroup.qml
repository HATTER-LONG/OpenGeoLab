pragma ComponentBehavior: Bound

import QtQuick

import "../theme"
import "../components" as Components

Item {
    id: groupRoot

    required property AppTheme theme
    required property var groupData
    required property int groupIndex
    required property int groupCount
    property int buttonSize: 68
    signal triggerAction(string actionKey)

    readonly property int actionCount: groupData.actions ? groupData.actions.length : 0

    function accentColor(name) {
        if (name === "accentB") {
            return theme.accentB;
        }
        if (name === "accentC") {
            return theme.accentC;
        }
        if (name === "accentD") {
            return theme.accentD;
        }
        if (name === "accentE") {
            return theme.accentE;
        }
        return theme.accentA;
    }

    width: Math.max(actionRow.implicitWidth + 20, actionCount * buttonSize + 20)
    height: parent ? parent.height : implicitHeight

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
            id: actionRow

            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 6

            Repeater {
                model: groupRoot.groupData.actions

                delegate: Components.RibbonTile {
                    required property var modelData

                    width: groupRoot.buttonSize
                    height: groupRoot.buttonSize
                    theme: groupRoot.theme
                    title: modelData.title
                    iconKind: modelData.icon
                    accentOne: groupRoot.accentColor(modelData.accentOne)
                    accentTwo: groupRoot.accentColor(modelData.accentTwo)
                    onClicked: groupRoot.triggerAction(modelData.key)
                }
            }
        }
    }

    Text {
        id: titleLabel

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 1
        text: groupRoot.groupData.title
        color: groupRoot.theme.textTertiary
        font.pixelSize: 9
        font.family: groupRoot.theme.bodyFontFamily
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
    }

    Rectangle {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 1
        color: groupRoot.theme.tint(groupRoot.theme.borderSubtle, 0.72)
        visible: groupRoot.groupIndex < groupRoot.groupCount - 1
    }
}