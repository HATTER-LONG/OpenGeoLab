pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import "../theme"

/**
 * @brief Floating viewport toolbar with view presets, fit, x-ray and mesh display toggles.
 *
 * Sits as a horizontal overlay at the top-right of the viewport.
 * Uses SVG icons for each button, matching the OGL reference design.
 */
Item {
    id: root

    required property AppTheme theme

    signal fitRequested
    signal presetRequested(int preset)
    signal xRayToggled
    signal showTessellationToggled

    property bool xRayActive: false
    property bool showTessellationActive: false

    implicitWidth: bar.implicitWidth + 20
    implicitHeight: 40

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: Qt.rgba(root.theme.bg0.r, root.theme.bg0.g, root.theme.bg0.b, 0.88)
        border.width: 1
        border.color: root.theme.borderSubtle
    }

    RowLayout {
        id: bar
        anchors.centerIn: parent
        spacing: 3

        ViewportToolButton {
            theme: root.theme
            iconKind: "viewFit"
            tooltip: qsTr("Fit to scene")
            onClicked: root.fitRequested()
        }

        Rectangle {
            width: 1; height: 20
            color: root.theme.borderSubtle
            Layout.alignment: Qt.AlignVCenter
        }

        ViewportToolButton {
            theme: root.theme
            iconKind: "viewFront"
            tooltip: qsTr("Front view")
            onClicked: root.presetRequested(0)
        }

        ViewportToolButton {
            theme: root.theme
            iconKind: "viewBack"
            tooltip: qsTr("Back view")
            onClicked: root.presetRequested(1)
        }

        ViewportToolButton {
            theme: root.theme
            iconKind: "viewTop"
            tooltip: qsTr("Top view")
            onClicked: root.presetRequested(2)
        }

        ViewportToolButton {
            theme: root.theme
            iconKind: "viewBottom"
            tooltip: qsTr("Bottom view")
            onClicked: root.presetRequested(3)
        }

        ViewportToolButton {
            theme: root.theme
            iconKind: "viewLeft"
            tooltip: qsTr("Left view")
            onClicked: root.presetRequested(4)
        }

        ViewportToolButton {
            theme: root.theme
            iconKind: "viewRight"
            tooltip: qsTr("Right view")
            onClicked: root.presetRequested(5)
        }

        Rectangle {
            width: 1; height: 20
            color: root.theme.borderSubtle
            Layout.alignment: Qt.AlignVCenter
        }

        ViewportToolButton {
            theme: root.theme
            iconKind: "viewXray"
            tooltip: qsTr("Toggle X-Ray mode")
            toggled: root.xRayActive
            onClicked: root.xRayToggled()
        }

        ViewportToolButton {
            theme: root.theme
            iconKind: "viewMesh"
            tooltip: qsTr("Toggle tessellation wireframe")
            toggled: root.showTessellationActive
            onClicked: root.showTessellationToggled()
        }
    }
}
