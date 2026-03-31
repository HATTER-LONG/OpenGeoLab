pragma ComponentBehavior: Bound

import QtQuick
import "../theme"
import "../components"
import OpenGeoLab.App

/** @brief Interactive 3D viewport powered by OpenGL. */
Item {
    id: root

    required property AppTheme theme

    /** @brief Expose the GLViewport so other panels can call its methods. */
    property alias glViewport: viewport

    GLViewport {
        id: viewport
        anchors.fill: parent
        pickingEnabled: true
        pickMode: 0

        onEntityPicked: (shapeId, entityType, localId) => {
            console.log(qsTr("Picked: Shape %1, Type %2, Local %3")
                .arg(shapeId)
                .arg(entityType)
                .arg(localId))
        }

        onEntityHovered: (shapeId, entityType, localId) => {
            // Future: update status bar.
        }

        onPickCleared: {
            // Future: clear selection UI.
        }
    }

    ViewportToolbar {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 12
        anchors.rightMargin: 12
        theme: root.theme
        xRayActive: viewport.xRayMode

        onFitRequested: viewport.fitToScene()
        onPresetRequested: (preset) => viewport.setViewPreset(preset)
        onXRayToggled: viewport.toggleXRay()
    }
}
