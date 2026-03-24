pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import "../theme"
import "../components"

/// @brief Left sidebar — scene explorer with box list.
Item {
    id: root

    required property AppTheme theme
    required property ListModel boxListModel

    implicitWidth: 280

    SectionCard {
        anchors.fill: parent
        anchors.margins: 0
        theme: root.theme
        title: qsTr("Scene")
        subtitle: qsTr("Explorer")

        Text {
            visible: root.boxListModel.count === 0
            width: parent.width
            text: qsTr("No geometry yet. Create a box to get started.")
            font.pixelSize: 13
            color: root.theme.textTertiary
            wrapMode: Text.WordWrap
        }

        ListView {
            visible: root.boxListModel.count > 0
            width: parent.width
            height: contentHeight
            model: root.boxListModel
            clip: true
            spacing: 4
            interactive: false

            delegate: BoxListItem {
                required property int index
                required property string label
                required property int boxId
                required property var center
                required property var size
                required property int vertexCount

                width: ListView.view.width
                theme: root.theme
            }
        }
    }
}