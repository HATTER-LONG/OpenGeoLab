/**
 * @file ViewToolbar.qml
 * @brief Viewport control toolbar with view preset buttons
 *
 * Provides buttons for camera view presets (Fit, Front, Top, Left, Right)
 * with icons. Positioned at the top-right of the viewport.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import OpenGeoLab 1.0

Rectangle {
    id: viewToolbar

    implicitWidth: buttonRow.implicitWidth + 16
    implicitHeight: 36
    radius: 6
    color: Qt.rgba(Theme.surfaceAlt.r, Theme.surfaceAlt.g, Theme.surfaceAlt.b, 0.85)
    border.width: 1
    border.color: Theme.border

    RowLayout {
        id: buttonRow
        anchors.centerIn: parent
        spacing: 4

        // Fit button
        ViewToolButton {
            iconSource: "qrc:/opengeolab/resources/icons/view_fit.svg"
            toolTipText: qsTr("Fit to scene (F)")
            onClicked: {
                // send fit request
            }
        }

        // Separator
        Rectangle {
            width: 1
            height: 20
            color: Theme.border
            Layout.alignment: Qt.AlignVCenter
        }

        // Front view
        ViewToolButton {
            iconSource: "qrc:/opengeolab/resources/icons/view_front.svg"
            toolTipText: qsTr("Front view")
            onClicked: {}
        }

        // Top view
        ViewToolButton {
            iconSource: "qrc:/opengeolab/resources/icons/view_top.svg"
            toolTipText: qsTr("Top view")
            onClicked: {}
        }

        // Left view
        ViewToolButton {
            iconSource: "qrc:/opengeolab/resources/icons/view_left.svg"
            toolTipText: qsTr("Left view")
            onClicked: {}
        }

        // Right view
        ViewToolButton {
            iconSource: "qrc:/opengeolab/resources/icons/view_right.svg"
            toolTipText: qsTr("Right view")
            onClicked: {}
        }
    }
}
