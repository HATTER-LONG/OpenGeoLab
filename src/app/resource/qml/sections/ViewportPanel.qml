pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab.Render
import "../theme"
import "../components"

/** @brief 3D viewport backed by OpenGL + orthographic camera. */
Item {
    id: root

    required property AppTheme theme

    ViewportItem {
        id: viewport
        anchors.fill: parent
    }

    // ── View toolbar overlay ───────────────────────────────────────────
    Rectangle {
        id: toolbarBg
        anchors {
            top: parent.top
            right: parent.right
            margins: 8
        }
        implicitWidth: toolbarRow.implicitWidth + 20
        implicitHeight: 48
        radius: 10
        color: Qt.rgba(root.theme.surfaceCard.r,
                       root.theme.surfaceCard.g,
                       root.theme.surfaceCard.b, 0.88)
        border.width: 1
        border.color: root.theme.borderDefault

        RowLayout {
            id: toolbarRow
            anchors.centerIn: parent
            spacing: 4

            ViewToolButton {
                theme: root.theme
                iconKind: "view_fit"
                toolTipText: qsTr("Fit to scene")
                onClicked: viewport.fitAll()
            }

            Rectangle { width: 1; height: 28; color: root.theme.borderSubtle; Layout.alignment: Qt.AlignVCenter }

            ViewToolButton {
                theme: root.theme
                iconKind: "view_front"
                toolTipText: qsTr("Front view")
                onClicked: viewport.setFrontView()
            }
            ViewToolButton {
                theme: root.theme
                iconKind: "view_back"
                toolTipText: qsTr("Back view")
                onClicked: viewport.setBackView()
            }
            ViewToolButton {
                theme: root.theme
                iconKind: "view_top"
                toolTipText: qsTr("Top view")
                onClicked: viewport.setTopView()
            }
            ViewToolButton {
                theme: root.theme
                iconKind: "view_bottom"
                toolTipText: qsTr("Bottom view")
                onClicked: viewport.setBottomView()
            }
            ViewToolButton {
                theme: root.theme
                iconKind: "view_left"
                toolTipText: qsTr("Left view")
                onClicked: viewport.setLeftView()
            }
            ViewToolButton {
                theme: root.theme
                iconKind: "view_right"
                toolTipText: qsTr("Right view")
                onClicked: viewport.setRightView()
            }
        }
    }
}
