pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0
import ".."

Item {
    id: root

    /// Currently selected entity type for picking
    property string selectedType: ""

    /// Whether pick mode is active
    property bool pickModeActive: false

    /// List of selected entities as [{type: "Face", uid: 123}, ...]
    property var selectedEntities: []

    /// Signal emitted when pick mode changes
    signal pickModeChanged(bool enabled, string entityType)

    /// Signal emitted when selection changes
    signal selectionChanged(var entities)

    implicitHeight: contentColumn.implicitHeight
    implicitWidth: 300

    /**
     * @brief Clear all selections
     */
    function clearSelection() {
        selectedEntities = [];
        selectionChanged(selectedEntities);
    }

    /**
     * @brief Add an entity to selection
     * @param entityType The type of entity (Vertex, Edge, Face, etc.)
     * @param entityUid The unique ID of the entity
     */
    function addSelection(entityType, entityUid) {
        // Check for duplicate
        for (let i = 0; i < selectedEntities.length; i++) {
            if (selectedEntities[i].type === entityType && selectedEntities[i].uid === entityUid) {
                return; // Already selected
            }
        }
        let newEntities = selectedEntities.slice();
        newEntities.push({
            type: entityType,
            uid: entityUid
        });
        selectedEntities = newEntities;
        selectionChanged(selectedEntities);
    }

    /**
     * @brief Remove an entity from selection
     * @param entityType The type of entity
     * @param entityUid The unique ID of the entity
     */
    function removeSelection(entityType, entityUid) {
        let newEntities = selectedEntities.filter(function (e) {
            return !(e.type === entityType && e.uid === entityUid);
        });
        selectedEntities = newEntities;
        selectionChanged(selectedEntities);
    }

    /**
     * @brief Activate pick mode for a specific entity type
     * @param entityType The type to pick (Vertex, Edge, Face, Solid, Part)
     */
    function activatePickMode(entityType) {
        if (selectedType === entityType && pickModeActive) {
            // Toggle off
            selectedType = "";
            pickModeActive = false;
        } else {
            selectedType = entityType;
            pickModeActive = true;
        }
        pickModeChanged(pickModeActive, selectedType);
    }

    /**
     * @brief Deactivate pick mode
     */
    function deactivatePickMode() {
        selectedType = "";
        pickModeActive = false;
        pickModeChanged(false, "");
    }

    Column {
        id: contentColumn
        width: parent.width
        spacing: 8

        // Entity type selector buttons
        Label {
            text: qsTr("Select Entity Type")
            font.pixelSize: 11
            color: Theme.textSecondary
        }

        RowLayout {
            width: parent.width
            spacing: 4

            EntityTypeButton {
                Layout.fillWidth: true
                entityType: "Vertex"
                iconSource: "qrc:/opengeolab/resources/icons/vertex.svg"
                selected: root.selectedType === "Vertex" && root.pickModeActive
                onClicked: root.activatePickMode("Vertex")
            }

            EntityTypeButton {
                Layout.fillWidth: true
                entityType: "Edge"
                iconSource: "qrc:/opengeolab/resources/icons/edge.svg"
                selected: root.selectedType === "Edge" && root.pickModeActive
                onClicked: root.activatePickMode("Edge")
            }

            EntityTypeButton {
                Layout.fillWidth: true
                entityType: "Face"
                iconSource: "qrc:/opengeolab/resources/icons/face.svg"
                selected: root.selectedType === "Face" && root.pickModeActive
                onClicked: root.activatePickMode("Face")
            }

            EntityTypeButton {
                Layout.fillWidth: true
                entityType: "Solid"
                iconSource: "qrc:/opengeolab/resources/icons/solid.svg"
                selected: root.selectedType === "Solid" && root.pickModeActive
                onClicked: root.activatePickMode("Solid")
            }

            EntityTypeButton {
                Layout.fillWidth: true
                entityType: "Part"
                iconSource: "qrc:/opengeolab/resources/icons/box.svg"
                selected: root.selectedType === "Part" && root.pickModeActive
                onClicked: root.activatePickMode("Part")
            }
        }

        // Pick mode indicator
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
                    text: qsTr("Click to select %1 • Right-click to cancel").arg(root.selectedType)
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
                                    text: modelData.type + ":" + modelData.uid
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

                                    onClicked: root.removeSelection(modelData.type, modelData.uid)
                                }
                            }
                        }
                    }
                }
            }
        }

        // Clear button
        Button {
            visible: root.selectedEntities.length > 0
            width: parent.width
            text: qsTr("Clear Selection")
            flat: true

            background: Rectangle {
                radius: 4
                color: parent.hovered ? Theme.hovered : "transparent"
                border.width: 1
                border.color: Theme.border
            }

            contentItem: Label {
                text: parent.text
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
    component EntityTypeButton: AbstractButton {
        id: typeBtn

        property string entityType: ""
        property string iconSource: ""
        property bool selected: false

        implicitHeight: 32
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

        contentItem: Column {
            anchors.centerIn: parent
            spacing: 2

            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 16
                height: 16
                source: typeBtn.iconSource
                sourceSize: Qt.size(16, 16)
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
