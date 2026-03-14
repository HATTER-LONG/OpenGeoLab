pragma ComponentBehavior: Bound

import QtQuick
import "../theme"

Rectangle {
    id: card

    required property AppTheme theme
    property string title: ""
    property string subtitle: ""
    property int introDelay: 0
    default property alias contentData: contentColumn.data

    radius: theme.radiusMedium
    color: theme.surfaceMuted
    border.width: 1
    border.color: theme.borderSubtle
    opacity: 0
    y: 12

    Component.onCompleted: introAnimation.start()

    ParallelAnimation {
        id: introAnimation

        running: false

        SequentialAnimation {
            PauseAnimation {
                duration: card.introDelay
            }

            NumberAnimation {
                target: card
                property: "opacity"
                from: 0
                to: 1
                duration: 220
                easing.type: Easing.OutCubic
            }
        }

        SequentialAnimation {
            PauseAnimation {
                duration: card.introDelay
            }

            NumberAnimation {
                target: card
                property: "y"
                from: 12
                to: 0
                duration: 260
                easing.type: Easing.OutCubic
            }
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Column {
            width: parent.width
            spacing: 3
            visible: card.title.length > 0 || card.subtitle.length > 0

            Text {
                visible: card.title.length > 0
                text: card.title
                color: card.theme.textPrimary
                font.pixelSize: 17
                font.bold: true
                font.family: card.theme.titleFontFamily
            }

            Text {
                visible: card.subtitle.length > 0
                width: parent.width
                text: card.subtitle
                wrapMode: Text.WordWrap
                color: card.theme.textSecondary
                font.pixelSize: 12
                font.family: card.theme.bodyFontFamily
            }
        }

        Column {
            id: contentColumn

            width: parent.width
            spacing: 10
        }
    }
}
