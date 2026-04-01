import QtQuick
import QtQuick.Layouts
import "../theme"

/**
 * Row of toggle buttons for selecting entity types (V/E/F/Solid).
 * V/E/F are combinable via bitmask OR. Solid is mutually exclusive with V/E/F.
 */
Item {
    id: root

    required property AppTheme theme

    /** @brief Current pick mask bitmask (matches Core::PickMask values). */
    property int mask: 7

    implicitWidth: row.implicitWidth
    implicitHeight: row.implicitHeight

    // Core::PickMask values
    readonly property int maskVertex: 1
    readonly property int maskEdge: 2
    readonly property int maskFace: 4
    readonly property int maskSolid: 16

    RowLayout {
        id: row

        anchors.fill: parent
        spacing: 4

        Repeater {
            model: [
                { label: qsTr("V"), mask: root.maskVertex },
                { label: qsTr("E"), mask: root.maskEdge },
                { label: qsTr("F"), mask: root.maskFace },
                { label: qsTr("Solid"), mask: root.maskSolid }
            ]

            delegate: Rectangle {
                id: toggleButton

                required property var modelData
                required property int index

                readonly property bool active: (root.mask & modelData.mask) !== 0
                readonly property bool isSolid: modelData.mask === root.maskSolid

                Layout.preferredWidth: isSolid ? 52 : 32
                Layout.preferredHeight: 28
                radius: root.theme.radiusSmall
                color: active
                    ? root.theme.tint(root.theme.accentE, root.theme.darkMode ? 0.28 : 0.18)
                    : root.theme.surfaceMuted
                border.width: 1
                border.color: active
                    ? root.theme.accentE
                    : root.theme.borderSubtle

                Behavior on color {
                    ColorAnimation { duration: root.theme.animFast }
                }

                Text {
                    anchors.centerIn: parent
                    text: toggleButton.modelData.label
                    color: toggleButton.active
                        ? root.theme.accentE
                        : root.theme.textSecondary
                    font.pixelSize: 12
                    font.bold: toggleButton.active
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        let newMask = root.mask;
                        if (toggleButton.isSolid) {
                            // Solid is exclusive with V/E/F
                            newMask = toggleButton.active ? 0 : root.maskSolid;
                        } else {
                            // V/E/F: toggle this bit, clear Solid
                            newMask = newMask & ~root.maskSolid;
                            newMask = newMask ^ toggleButton.modelData.mask;
                        }
                        if (newMask === 0) {
                            return; // Don't allow empty mask
                        }
                        root.mask = newMask;
                    }
                }
            }
        }
    }
}
