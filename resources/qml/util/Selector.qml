pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0
import ".."

Item {
    id: root

    implicitHeight: contentColumn.implicitHeight
    implicitWidth: 300

    readonly property int none_type: 0
    readonly property int vertex_type: 1
    readonly property int edge_type: 2
    readonly property int face_type: 4
    readonly property int solid_type: 8
    readonly property int part_type: 16

    property int selectedTypes: none_type

    property bool pickModeActive: false

    property var selectedEntities: []

    function isEntityTypeVisible(type) {
        return true;
    }
    onVisibleChanged: {
        if (!visible) {
            SelectManagerService.clearSelection();
            SelectManagerService.activateSelectMode(root.none_type);
            SelectManagerService.deactivateSelectMode();
        }
    }
    function onEntityTypeClicked(type) {
        var current = root.selectedTypes;
        if ((current & type) === 0) {
            if (type === root.part_type || type === root.solid_type) {
                current = root.none_type;
            } else if (type === root.vertex_type || type === root.edge_type || type === root.face_type) {
                current &= ~(root.solid_type | root.part_type);
            }
            current |= type;
        } else {
            current &= ~type;
        }
        if (current !== root.none_type) {
            SelectManagerService.activateSelectMode(current);
        } else {
            SelectManagerService.activateSelectMode(root.none_type);
            SelectManagerService.deactivateSelectMode();
        }
    }

    function updateSelectedEntities() {
        root.selectedEntities = SelectManagerService.currentSelections();
    }
    function removeSelection(uid, type_str) {
        SelectManagerService.removeEntity(uid, type_str);
    }
    function clearSelection() {
        SelectManagerService.clearSelection();
    }

    Connections {
        target: SelectManagerService
        function onSelectModeChanged(types) {
            root.selectedTypes = types;
        }
        function onSelectModeActivated(enabled) {
            console.log("Pick mode activated: " + enabled);
            root.pickModeActive = enabled;
        }
        function onEntitySelected(uid, type_str) {
            root.selectedEntities = root.selectedEntities.concat([
                {
                    uid: uid,
                    type: type_str
                }
            ]);
        }
        function onEntityRemoved(uid, type) {
            root.selectedEntities = root.selectedEntities.filter(function (entity) {
                return !(entity.uid === uid && entity.type === type);
            });
        }
        function onSelectionCleared() {
            root.selectedEntities = [];
        }
    }
    Column {
        id: contentColumn
        width: parent.width
        spacing: 8

        Label {
            text: qsTr("Select Entity Type")
            font.pixelSize: 11
            font.bold: true
            color: Theme.textSecondary
            visible: true
        }
        RowLayout {
            width: parent.width
            spacing: 4
            visible: true

            EntityTypeButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 56
                visible: root.isEntityTypeVisible(root.vertex_type)
                entityType: "Vertex"
                iconSource: "qrc:/opengeolab/resources/icons/vertex.svg"
                selected: ((root.selectedTypes & root.vertex_type) !== 0)
                onClicked: root.onEntityTypeClicked(root.vertex_type)
            }

            EntityTypeButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 56
                visible: root.isEntityTypeVisible(root.edge_type)
                entityType: "Edge"
                iconSource: "qrc:/opengeolab/resources/icons/edge.svg"
                selected: ((root.selectedTypes & root.edge_type) !== 0)
                onClicked: root.onEntityTypeClicked(root.edge_type)
            }

            EntityTypeButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 56
                visible: root.isEntityTypeVisible(root.face_type)
                entityType: "Face"
                iconSource: "qrc:/opengeolab/resources/icons/face.svg"
                selected: ((root.selectedTypes & root.face_type) !== 0)
                onClicked: root.onEntityTypeClicked(root.face_type)
            }

            EntityTypeButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 56
                visible: root.isEntityTypeVisible(root.solid_type)
                entityType: "Solid"
                iconSource: "qrc:/opengeolab/resources/icons/solid.svg"
                selected: ((root.selectedTypes & root.solid_type) !== 0)
                onClicked: root.onEntityTypeClicked(root.solid_type)
            }

            EntityTypeButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 56
                visible: root.isEntityTypeVisible(root.part_type)
                entityType: "Part"
                iconSource: "qrc:/opengeolab/resources/icons/box.svg"
                selected: ((root.selectedTypes & root.part_type) !== 0)
                onClicked: root.onEntityTypeClicked(root.part_type)
            }
        }
        Rectangle {
            visible: root.pickModeActive
            width: parent.width
            height: pickModeRow.implicitHeight + 8
            radius: 4
            color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15)
            border.width: 1
            border.color: Theme.accent

            RowLayout {
                id: pickModeRow
                anchors.fill: parent
                anchors.margins: 4
                spacing: 6

                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: Theme.accent

                    SequentialAnimation on opacity {
                        running: root.pickModeActive
                        loops: Animation.Infinite
                        NumberAnimation {
                            to: 0.3
                            duration: 800
                        }
                        NumberAnimation {
                            to: 1.0
                            duration: 800
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Click to select • Right-click to cancel")
                    font.pixelSize: 10
                    color: Theme.accent
                    elide: Text.ElideRight
                }
            }
        }

        // Selected entities display
        Label {
            visible: root.selectedEntities.length > 0
            text: qsTr("Selected Entities (%1)").arg(root.selectedEntities.length)
            font.pixelSize: 11
            color: Theme.textSecondary
        }

        Rectangle {
            visible: root.selectedEntities.length > 0
            width: parent.width
            height: Math.min(selectionFlow.implicitHeight + 12, 100)
            radius: 4
            color: Theme.surfaceAlt
            clip: true

            Flickable {
                anchors.fill: parent
                anchors.margins: 6
                contentHeight: selectionFlow.implicitHeight
                clip: true

                Flow {
                    id: selectionFlow
                    width: parent.width
                    spacing: 4

                    Repeater {
                        model: root.selectedEntities

                        delegate: Rectangle {
                            id: selectionChip
                            required property var modelData
                            required property int index

                            height: 22
                            width: entityLabel.implicitWidth + closeBtn.width + 12
                            radius: 3
                            color: Theme.surface
                            border.width: 1
                            border.color: Theme.border

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 2
                                spacing: 2

                                Label {
                                    id: entityLabel
                                    text: selectionChip.modelData.type + ":" + selectionChip.modelData.uid

                                    font.pixelSize: 10
                                    font.family: "Consolas, monospace"
                                    color: Theme.textPrimary
                                }

                                AbstractButton {
                                    id: closeBtn
                                    Layout.preferredWidth: 16
                                    Layout.preferredHeight: 16
                                    hoverEnabled: true

                                    background: Rectangle {
                                        radius: 2
                                        color: closeBtn.hovered ? Theme.danger : "transparent"
                                    }

                                    contentItem: Label {
                                        text: "×"
                                        font.pixelSize: 12
                                        font.bold: true
                                        color: closeBtn.hovered ? Theme.white : Theme.textSecondary
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    onClicked: root.removeSelection(selectionChip.modelData.uid, selectionChip.modelData.type)
                                }
                            }
                        }
                    }
                }
            }
        }
        // Clear button
        Button {
            id: clearBtn
            visible: root.selectedEntities.length > 0
            width: parent.width
            text: qsTr("Clear Selection")
            flat: true

            background: Rectangle {
                radius: 4
                color: clearBtn.hovered ? Theme.hovered : "transparent"
                border.width: 1
                border.color: Theme.border
            }

            contentItem: Label {
                text: clearBtn.text
                font.pixelSize: 11
                color: Theme.textSecondary
                horizontalAlignment: Text.AlignHCenter
            }

            onClicked: root.clearSelection()
        }

        // Empty state hint
        Rectangle {
            visible: root.selectedEntities.length === 0 && !root.pickModeActive
            width: parent.width
            height: hintLabel.implicitHeight + 12
            radius: 4
            color: Theme.surfaceAlt

            Label {
                id: hintLabel
                anchors.fill: parent
                anchors.margins: 6
                text: qsTr("Click an entity type button above to start picking")
                font.pixelSize: 10
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
    // =========================================================
    // Internal entity type button component
    // =========================================================

    /**
     * @brief Button for selecting entity type in pick mode
     *
     * Displays icon and label with selection highlight.
     * All buttons have uniform width for consistent layout.
     */
    component EntityTypeButton: AbstractButton {
        id: typeBtn

        /// Entity type identifier (Vertex, Edge, Face, Solid, Part)
        property string entityType: ""

        /// Icon source URL
        property string iconSource: ""

        /// Whether this type is currently selected
        property bool selected: false

        implicitHeight: 40
        implicitWidth: 56
        hoverEnabled: true

        ToolTip.visible: hovered
        ToolTip.text: qsTr("Select %1").arg(entityType)
        ToolTip.delay: 500

        background: Rectangle {
            radius: 4
            color: typeBtn.selected ? Theme.accent : typeBtn.pressed ? Theme.clicked : typeBtn.hovered ? Theme.hovered : Theme.surface
            border.width: 1
            border.color: typeBtn.selected ? Theme.accent : Theme.border

            Behavior on color {
                ColorAnimation {
                    duration: 150
                }
            }
        }

        contentItem: Item {
            anchors.fill: parent

            Column {
                anchors.centerIn: parent
                spacing: 3

                ThemedIcon {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 16
                    height: 16
                    source: typeBtn.iconSource
                    opacity: typeBtn.selected ? 1.0 : 0.7
                }

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: typeBtn.entityType
                    font.pixelSize: 9
                    font.bold: typeBtn.selected
                    color: typeBtn.selected ? Theme.white : Theme.textSecondary
                }
            }
        }
    }
}
