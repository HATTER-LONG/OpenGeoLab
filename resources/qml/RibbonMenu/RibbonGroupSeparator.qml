pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import OpenGeoLab 1.0

Rectangle {
    id: ribbonGroupSeparator
    width: 2
    height: parent.height

    anchors.verticalCenter: parent.verticalCenter

    readonly property bool darkMode: Material.theme === Material.Dark
    color: darkMode ? Qt.rgba(1, 1, 1, 0.18) : Qt.rgba(0, 0, 0, 0.18)
}
