pragma ComponentBehavior: Bound

import QtQuick
import "../theme"

Rectangle {
    id: panel

    required property AppTheme theme
    property string title: ""
    property string bodyText: ""
    property int minimumBodyHeight: 120
    property int maximumBodyHeight: 260
    readonly property int bodyHeight: Math.min(maximumBodyHeight, Math.max(minimumBodyHeight, codeText.paintedHeight + 24))

    radius: theme.radiusMedium
    color: theme.surfaceMuted
    border.width: 1
    border.color: theme.borderSubtle
    implicitHeight: titleLabel.implicitHeight + bodyHeight + 48

    Column {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        Text {
            id: titleLabel

            text: panel.title
            color: panel.theme.textSecondary
            font.pixelSize: 13
            font.bold: true
            font.family: panel.theme.bodyFontFamily
        }

        Rectangle {
            width: parent.width
            height: panel.bodyHeight
            radius: panel.theme.radiusSmall
            color: panel.theme.tint(panel.theme.surface, panel.theme.darkMode ? 0.64 : 0.9)
            border.width: 1
            border.color: panel.theme.tint(panel.theme.borderSubtle, 0.75)

            Flickable {
                anchors.fill: parent
                anchors.margins: 12
                contentWidth: width
                contentHeight: codeText.paintedHeight
                clip: true

                Text {
                    id: codeText

                    width: parent.width
                    text: panel.bodyText && panel.bodyText.length > 0 ? panel.bodyText : qsTr("No output yet.")
                    color: panel.theme.textPrimary
                    font.pixelSize: 12
                    font.family: panel.theme.monoFontFamily
                    wrapMode: Text.WrapAnywhere
                }
            }
        }
    }
}
