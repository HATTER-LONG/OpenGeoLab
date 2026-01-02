pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * @file GeometrySelector.qml
 * @brief Geometry selector component for picking geometry elements
 *
 * Provides a UI control for selecting geometry elements from the 3D viewport.
 * Supports selection of vertices, edges, faces, and parts.
 *
 * Features:
 * - Type selection dropdown (Vertex/Edge/Face/Part)
 * - Visual feedback for selection state
 * - Integration with viewport selection mode
 * - Clear selection functionality
 */
Item {
    id: root

    /**
     * @brief Selection type (1=Vertex, 2=Edge, 3=Face, 4=Part)
     */
    property int selectionType: 1

    /**
     * @brief ID of the currently selected geometry element
     */
    property int selectedId: 0

    /**
     * @brief Display text for the selected element
     */
    property string selectedInfo: ""

    /**
     * @brief Whether the selector is actively listening for viewport selection
     */
    property bool isSelecting: false

    /**
     * @brief Label text displayed above the selector
     */
    property string label: qsTr("Select Geometry:")

    /**
     * @brief Placeholder text when nothing is selected
     */
    property string placeholder: qsTr("Click to select...")

    /**
     * @brief Read-only mode disables selection
     */
    property bool readOnly: false

    /**
     * @brief Emitted when selection changes
     * @param id Selected element ID
     * @param type Selection type
     */
    signal selectionChanged(int id, int type)

    implicitWidth: 300
    implicitHeight: contentLayout.implicitHeight

    ColumnLayout {
        id: contentLayout
        anchors.fill: parent
        spacing: 8

        // Label
        Label {
            Layout.fillWidth: true
            text: root.label
            color: Theme.textPrimaryColor
            font.pixelSize: 13
        }

        // Selection types row
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Label {
                text: qsTr("Type:")
                color: Theme.textSecondaryColor
                font.pixelSize: 12
            }

            ComboBox {
                id: typeCombo
                Layout.fillWidth: true
                model: [qsTr("Vertex"), qsTr("Edge"), qsTr("Face"), qsTr("Part")]
                currentIndex: Math.max(0, root.selectionType - 1)
                enabled: !root.readOnly && !root.isSelecting
                onCurrentIndexChanged: {
                    root.selectionType = currentIndex + 1;
                }

                background: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 32
                    color: typeCombo.enabled ? Theme.surfaceColor : Theme.surfaceAltColor
                    border.width: 1
                    border.color: typeCombo.pressed ? Theme.accentColor : Theme.borderColor
                    radius: 4
                }
            }
        }

        // Selection area
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: root.isSelecting ? Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.1) : Theme.surfaceAltColor
            radius: 6
            border.width: root.isSelecting ? 2 : 1
            border.color: root.isSelecting ? Theme.accentColor : Theme.borderColor

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                // Selection icon
                Rectangle {
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    color: root.selectedId > 0 ? Theme.accentColor : "transparent"
                    radius: 4
                    border.width: root.selectedId > 0 ? 0 : 1
                    border.color: Theme.borderColor

                    Label {
                        anchors.centerIn: parent
                        text: root.selectedId > 0 ? "✓" : getTypeIcon()
                        font.pixelSize: 16
                        color: root.selectedId > 0 ? "white" : Theme.textSecondaryColor

                        function getTypeIcon() {
                            switch (root.selectionType) {
                            case 1:
                                return "•";  // Vertex
                            case 2:
                                return "—";  // Edge
                            case 3:
                                return "▢";  // Face
                            case 4:
                                return "⬡";  // Part
                            default:
                                return "?";
                            }
                        }
                    }
                }

                // Selection info
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        Layout.fillWidth: true
                        text: root.selectedId > 0 ? root.selectedInfo : root.placeholder
                        color: root.selectedId > 0 ? Theme.textPrimaryColor : Theme.textSecondaryColor
                        font.pixelSize: 13
                        font.italic: root.selectedId === 0
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.selectedId > 0 ? qsTr("ID: %1").arg(root.selectedId) : ""
                        color: Theme.textSecondaryColor
                        font.pixelSize: 11
                        visible: root.selectedId > 0
                    }
                }

                // Action buttons
                RowLayout {
                    spacing: 4

                    Button {
                        id: pickButton
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 32
                        flat: true
                        enabled: !root.readOnly
                        icon.source: root.isSelecting ? "qrc:/opengeolab/resources/icons/close.svg" : "qrc:/opengeolab/resources/icons/target.svg"
                        icon.color: Theme.textPrimaryColor

                        ToolTip.text: root.isSelecting ? qsTr("Cancel") : qsTr("Pick from viewport")
                        ToolTip.visible: pickButton.hovered

                        onClicked: {
                            root.isSelecting = !root.isSelecting;
                        }

                        background: Rectangle {
                            color: pickButton.hovered ? Theme.accentColor : "transparent"
                            radius: 4
                            opacity: 0.2
                        }
                    }

                    Button {
                        id: clearButton
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 32
                        flat: true
                        enabled: !root.readOnly && root.selectedId > 0
                        icon.source: "qrc:/opengeolab/resources/icons/delete.svg"
                        icon.color: enabled ? Theme.errorColor : Theme.textSecondaryColor

                        ToolTip.text: qsTr("Clear selection")
                        ToolTip.visible: hovered

                        onClicked: {
                            root.clearSelection();
                        }

                        background: Rectangle {
                            color: clearButton.hovered ? Theme.errorColor : "transparent"
                            radius: 4
                            opacity: 0.2
                        }
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                enabled: !root.readOnly && !root.isSelecting
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.isSelecting = true;
                }
            }
        }

        // Selection hint
        Label {
            Layout.fillWidth: true
            text: root.isSelecting ? qsTr("Click on geometry in the viewport to select") : ""
            color: Theme.accentColor
            font.pixelSize: 11
            font.italic: true
            visible: root.isSelecting
        }
    }

    // Public methods
    function setSelection(id, info) {
        root.selectedId = id;
        root.selectedInfo = info || qsTr("Selected element");
        root.isSelecting = false;
        root.selectionChanged(id, root.selectionType);
    }

    function clearSelection() {
        root.selectedId = 0;
        root.selectedInfo = "";
        root.selectionChanged(0, root.selectionType);
    }

    function startSelection() {
        root.isSelecting = true;
    }

    function stopSelection() {
        root.isSelecting = false;
    }
}
