pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import "../theme"

/// @brief Content card with optional title, subtitle, and a default content slot.
Item {
    id: root

    required property AppTheme theme
    property string title: ""
    property string subtitle: ""
    default property alias content: contentColumn.data

    implicitWidth: 260
    implicitHeight: cardLayout.implicitHeight + 2 * root.theme.shellPadding

    Rectangle {
        anchors.fill: parent
        radius: root.theme.radiusMedium
        color: root.theme.surface
        border.width: 1
        border.color: root.theme.borderSubtle
    }

    ColumnLayout {
        id: cardLayout
        anchors.fill: parent
        anchors.margins: root.theme.shellPadding
        spacing: root.theme.gapTight

        Text {
            visible: root.title.length > 0
            text: root.title
            font.pixelSize: 14
            font.weight: Font.DemiBold
            color: root.theme.textPrimary
        }

        Text {
            visible: root.subtitle.length > 0
            text: root.subtitle
            font.pixelSize: 12
            color: root.theme.textTertiary
        }

        Column {
            id: contentColumn
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: root.theme.gapTight
        }
    }
}
