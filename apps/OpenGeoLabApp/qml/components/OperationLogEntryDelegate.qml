pragma ComponentBehavior: Bound

import QtQuick

import "../theme"

Rectangle {
    id: entry

    required property AppTheme theme
    required property int level
    required property string levelName
    required property string source
    required property string message
    required property string time
    required property int threadId
    required property string file
    required property int line
    required property string functionName
    readonly property color accentColor: {
        if (level >= 4) {
            return theme.accentD;
        }
        if (level === 3) {
            return theme.accentC;
        }
        if (level === 2) {
            return theme.accentB;
        }
        return theme.accentA;
    }
    readonly property string secondaryMeta: {
        const meta = [];
        if (threadId > 0) {
            meta.push(qsTr("tid %1").arg(threadId));
        }
        if (file.length > 0) {
            meta.push(line > 0 ? qsTr("%1:%2").arg(file).arg(line) : file);
        }
        return meta.join(" \u00b7 ");
    }

    radius: theme.radiusSmall
    color: theme.tint(theme.surface, theme.darkMode ? 0.72 : 0.98)
    border.width: 1
    border.color: theme.tint(accentColor, theme.darkMode ? 0.36 : 0.2)
    implicitHeight: contentColumn.implicitHeight + 18

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 7
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 7
        width: 4
        radius: 2
        color: entry.accentColor
    }

    Column {
        id: contentColumn

        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 10
        anchors.topMargin: 9
        anchors.bottomMargin: 9
        spacing: 6

        Row {
            width: parent.width
            spacing: 8

            Rectangle {
                id: levelChip

                width: levelLabel.implicitWidth + 14
                height: 20
                radius: 8
                color: entry.theme.tint(entry.accentColor, entry.theme.darkMode ? 0.24 : 0.14)
                border.width: 1
                border.color: entry.theme.tint(entry.accentColor, entry.theme.darkMode ? 0.5 : 0.28)

                Text {
                    id: levelLabel

                    anchors.centerIn: parent
                    text: entry.levelName.toUpperCase()
                    color: entry.theme.textPrimary
                    font.pixelSize: 10
                    font.bold: true
                    font.family: entry.theme.bodyFontFamily
                }
            }

            Text {
                width: Math.max(0, parent.width - levelChip.width - timeLabel.implicitWidth - 20)
                text: entry.source
                color: entry.theme.textSecondary
                font.pixelSize: 11
                font.bold: true
                font.family: entry.theme.bodyFontFamily
                elide: Text.ElideRight
            }

            Text {
                id: timeLabel

                text: entry.time
                color: entry.theme.textTertiary
                font.pixelSize: 10
                font.family: entry.theme.monoFontFamily
            }
        }

        Text {
            width: parent.width
            text: entry.message
            wrapMode: Text.Wrap
            color: entry.theme.textPrimary
            font.pixelSize: 12
            font.family: entry.theme.bodyFontFamily
        }

        Text {
            visible: entry.secondaryMeta.length > 0
            width: parent.width
            text: entry.secondaryMeta
            wrapMode: Text.Wrap
            color: entry.theme.textTertiary
            font.pixelSize: 10
            font.family: entry.theme.bodyFontFamily
        }
    }
}
