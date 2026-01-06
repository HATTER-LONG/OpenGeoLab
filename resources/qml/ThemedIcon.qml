import QtQuick
import QtQuick.Window
import Qt5Compat.GraphicalEffects
import OpenGeoLab 1.0

Item {
    id: root

    property url source
    property color color: Theme.palette.text
    property int size: 18

    implicitWidth: size
    implicitHeight: size

    width: implicitWidth
    height: implicitHeight

    readonly property int _pixelWidth: Math.max(1, Math.round(width * Screen.devicePixelRatio))
    readonly property int _pixelHeight: Math.max(1, Math.round(height * Screen.devicePixelRatio))

    Image {
        id: img
        anchors.fill: parent
        source: root.source
        fillMode: Image.PreserveAspectFit
        visible: false
        smooth: true
        antialiasing: true
        cache: true

        sourceSize.width: root._pixelWidth
        sourceSize.height: root._pixelHeight
    }

    ColorOverlay {
        anchors.fill: img
        source: img
        color: root.color
    }
}
