pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Effects
import "../theme"

Item {
    id: root

    required property AppTheme theme
    property alias text: label.text
    property bool shown: false

    visible: opacity > 0
    opacity: 0
    z: 1

    implicitWidth: bubble.implicitWidth
    implicitHeight: bubble.implicitHeight
    width: implicitWidth
    height: implicitHeight

    states: [
        State {
            name: "hidden"
            when: !root.shown

            PropertyChanges {
                target: root
                opacity: 0
            }
        },
        State {
            name: "shown"
            when: root.shown

            PropertyChanges {
                target: root
                opacity: 1
            }
        }
    ]

    Behavior on opacity {
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutCubic
        }
    }

    Item {
        id: bubble
        implicitWidth: card.implicitWidth
        implicitHeight: card.y + card.implicitHeight
        width: implicitWidth
        height: implicitHeight
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowBlur: 0.4
            shadowVerticalOffset: 4
            shadowColor: Qt.rgba(0, 0, 0, 0.25)
        }

        Rectangle {
            id: card
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 6
            implicitWidth: label.implicitWidth + 20
            implicitHeight: label.implicitHeight + 14
            radius: 8
            color: root.theme.surfaceMuted
            border.width: 1
            border.color: root.theme.borderSubtle
            antialiasing: true

            Rectangle {
                id: arrow
                width: 10
                height: 10
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: -5
                rotation: 45
                color: root.theme.surfaceMuted
                border.width: 1
                border.color: root.theme.borderSubtle
                antialiasing: true
            }

            Rectangle {
                width: arrow.width + 6
                height: 6
                anchors.horizontalCenter: arrow.horizontalCenter
                anchors.top: parent.top
                color: root.theme.surfaceMuted
            }

            Text {
                id: label
                anchors.centerIn: parent
                font.pixelSize: 13
                font.weight: Font.Medium
                color: root.theme.textPrimary
            }
        }
    }
}
