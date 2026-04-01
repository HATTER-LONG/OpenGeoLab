pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "../theme"

/**
 * Row of toggle buttons for selecting entity types (Vertex/Edge/Face/Solid).
 *
 * Each button displays an SVG icon and label with selection highlight.
 * V/E/F are combinable via bitmask OR. Solid is mutually exclusive with V/E/F.
 *
 * Visual design follows OGL Selector.qml conventions:
 * - 44-height buttons with icon + label
 * - Accent-tinted background on selection
 * - Tooltip on hover, color animation transitions
 */
Item {
    id: root

    required property AppTheme theme

    /** @brief Current pick mask bitmask (matches Core::PickMask values). */
    property int mask: 11

    implicitWidth: row.implicitWidth
    implicitHeight: row.implicitHeight

    // Core::PickMask values (must match C++ enum in pick_mask.hpp)
    readonly property int maskVertex: 1   // 1 << 0
    readonly property int maskEdge: 2     // 1 << 1
    readonly property int maskFace: 8     // 1 << 3
    readonly property int maskSolid: 16   // 1 << 4

    RowLayout {
        id: row

        anchors.fill: parent
        spacing: 4

        Repeater {
            model: [
                { label: qsTr("Vertex"), icon: "entityVertex", mask: root.maskVertex },
                { label: qsTr("Edge"),   icon: "entityEdge",   mask: root.maskEdge },
                { label: qsTr("Face"),   icon: "entityFace",   mask: root.maskFace },
                { label: qsTr("Solid"),  icon: "entitySolid",  mask: root.maskSolid }
            ]

            delegate: AbstractButton {
                id: typeBtn

                required property var modelData
                required property int index

                readonly property bool selected: (root.mask & modelData.mask) !== 0
                readonly property bool isSolid: modelData.mask === root.maskSolid

                Layout.preferredWidth: 56
                Layout.preferredHeight: 44
                hoverEnabled: true

                ToolTip.visible: hovered
                ToolTip.text: qsTr("Select %1").arg(typeBtn.modelData.label)
                ToolTip.delay: 500

                background: Rectangle {
                    radius: root.theme.radiusSmall
                    color: typeBtn.selected
                        ? root.theme.tint(root.theme.accentA,
                            root.theme.darkMode ? 0.24 : 0.14)
                        : typeBtn.pressed
                            ? root.theme.surfaceStrong
                            : typeBtn.hovered
                                ? root.theme.surfaceMuted
                                : root.theme.surface
                    border.width: 1
                    border.color: typeBtn.selected
                        ? root.theme.accentA
                        : typeBtn.hovered
                            ? root.theme.tint(root.theme.accentA,
                                root.theme.darkMode ? 0.4 : 0.3)
                            : root.theme.borderSubtle

                    Behavior on color {
                        ColorAnimation { duration: root.theme.animFast }
                    }

                    Behavior on border.color {
                        ColorAnimation { duration: root.theme.animFast }
                    }
                }

                contentItem: Item {
                    anchors.fill: parent

                    Column {
                        anchors.centerIn: parent
                        spacing: 3

                        AppIcon {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: 20
                            height: 20
                            theme: root.theme
                            iconKind: typeBtn.modelData.icon
                            useThemeContrast: false
                            primaryColor: typeBtn.selected
                                ? root.theme.accentA
                                : root.theme.textSecondary
                            opacity: typeBtn.selected ? 1.0 : 0.7
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: typeBtn.modelData.label
                            font.pixelSize: 9
                            font.bold: typeBtn.selected
                            color: typeBtn.selected
                                ? root.theme.accentA
                                : root.theme.textSecondary
                        }
                    }
                }

                onClicked: {
                    let newMask = root.mask;
                    if (typeBtn.isSolid) {
                        newMask = typeBtn.selected ? 0 : root.maskSolid;
                    } else {
                        newMask = newMask & ~root.maskSolid;
                        newMask = newMask ^ typeBtn.modelData.mask;
                    }
                    if (newMask === 0) {
                        return;
                    }
                    root.mask = newMask;
                }
            }
        }
    }
}
