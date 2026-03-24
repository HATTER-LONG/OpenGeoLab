pragma ComponentBehavior: Bound

import QtQuick
import OpenGeoLab.App
import "../theme"

/// @brief 3D viewport panel using GLViewportItem for OpenGL rendering.
Item {
    id: root

    required property AppTheme theme

    GLViewportItem {
        id: viewport
        anchors.fill: parent
    }

    Connections {
        target: viewport.controller

        function onCameraChanged() {
            viewport.update()
        }
    }
}
