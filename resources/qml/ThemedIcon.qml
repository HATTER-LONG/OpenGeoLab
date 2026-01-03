import QtQuick
import Qt5Compat.GraphicalEffects
import OpenGeoLab 1.0

Item {
    id: root

    property url source
    property color color: Theme.palette.text
    property int size: 18

    implicitWidth: size
    implicitHeight: size

    Image {
        id: img
        anchors.fill: parent
        source: root.source
        fillMode: Image.PreserveAspectFit
        visible: false
        smooth: true
        antialiasing: true
    }

    ColorOverlay {
        anchors.fill: img
        source: img
        color: root.color
    }
}
