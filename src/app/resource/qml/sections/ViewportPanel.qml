pragma ComponentBehavior: Bound

import QtQuick
import Qt5Compat.GraphicalEffects
import "../theme"
import "../components"
import OpenGeoLab.App

/** @brief Interactive 3D viewport powered by OpenGL, with rounded-corner mask. */
Item {
    id: root

    required property AppTheme theme

    /** @brief Expose the GLViewport so other panels can call its methods. */
    property alias glViewport: viewport

    /** @brief Rounded-corner container that masks the FBO output. */
    Item {
        id: viewportContainer
        anchors.fill: parent
        layer.enabled: true
        layer.effect: OpacityMask {
            maskSource: Rectangle {
                width: viewportContainer.width
                height: viewportContainer.height
                radius: root.theme.radiusMedium
            }
        }

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
    }

    /** @brief Rounded border drawn on top of the masked viewport. */
    Rectangle {
        anchors.fill: parent
        radius: root.theme.radiusMedium
        color: "transparent"
        border.width: 1
        border.color: root.theme.borderSubtle
    }

    /** @brief Box-selection rubber-band overlay. */
    Rectangle {
        id: rubberBand

        visible: viewport.boxSelectActive
        x: viewport.boxSelectRect.x
        y: viewport.boxSelectRect.y
        width: viewport.boxSelectRect.width
        height: viewport.boxSelectRect.height
        color: "transparent"
        border.width: 1
        border.color: root.theme.accentA
        opacity: 0.8

        Rectangle {
            anchors.fill: parent
            color: root.theme.accentA
            opacity: 0.12
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
