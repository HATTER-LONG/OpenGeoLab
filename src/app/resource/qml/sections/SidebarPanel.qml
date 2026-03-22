pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import "../theme"
import "../components"

/// @brief Left sidebar placeholder — scene tree and tools will live here.
Item {
    id: root

    required property AppTheme theme

    implicitWidth: 280

    SectionCard {
        anchors.fill: parent
        anchors.margins: 0
        theme: root.theme
        title: qsTr("Scene")
        subtitle: qsTr("Explorer")

        Text {
            width: parent.width
            text: qsTr("Scene tree and tools will appear here.")
            font.pixelSize: 13
            color: root.theme.textTertiary
            wrapMode: Text.WordWrap
        }
    }
}
