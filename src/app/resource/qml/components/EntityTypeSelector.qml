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
    property var typeModel: [
        { label: qsTr("Vertex"), icon: "entityVertex", mask: root.maskVertex },
        { label: qsTr("Edge"),   icon: "entityEdge",   mask: root.maskEdge },
        { label: qsTr("Face"),   icon: "entityFace",   mask: root.maskFace },
        { label: qsTr("Solid"),  icon: "entitySolid",  mask: root.maskSolid }
    ]
    /** @brief Masks that are mutually exclusive with all other types. Default: Solid. */
    property var exclusiveMasks: [root.maskSolid]

    implicitWidth: row.implicitWidth
    implicitHeight: row.implicitHeight

    // Core::PickMask values (must match C++ enum in pick_mask.hpp)
    readonly property int maskVertex: 1   // 1 << 0
    readonly property int maskEdge: 2     // 1 << 1
    readonly property int maskFace: 8     // 1 << 3
    readonly property int maskSolid: 16   // 1 << 4
    readonly property int maskMeshNode: 64      // 1 << 6
    readonly property int maskMeshEdge: 128     // 1 << 7
    readonly property int maskMeshElement: 256  // 1 << 8

    RowLayout {
        id: row

        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 2

        Repeater {
            model: root.typeModel

            delegate: AbstractButton {
                id: typeBtn

                required property var modelData
                required property int index

                readonly property bool selected: (root.mask & modelData.mask) !== 0
                readonly property bool isExclusive: root.exclusiveMasks.indexOf(modelData.mask) >= 0

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
                    border.width: typeBtn.selected ? 1.5 : 1
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
                    if (typeBtn.isExclusive) {
                        newMask = typeBtn.selected ? 0 : typeBtn.modelData.mask;
                    } else {
                        for (let i = 0; i < root.exclusiveMasks.length; ++i) {
                            newMask = newMask & ~root.exclusiveMasks[i];
                        }
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
