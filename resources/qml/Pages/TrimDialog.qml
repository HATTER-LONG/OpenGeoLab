pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "." as Pages

/**
 * @brief Non-modal dialog for trimming geometry
 *
 * This dialog allows users to:
 * - Select geometry to trim using the GeometrySelector component
 * - Choose trim mode and options
 * - Apply trim operation while still interacting with the main viewport
 */
Item {
    id: root

    property var initialParams: ({})
    property bool isVisible: false

    // Reference to viewport for selection integration
    property var viewport: null

    // Signals
    signal closeRequested

    visible: isVisible
    width: 320
    height: contentColumn.implicitHeight + 32

    // Dialog container
    Rectangle {
        anchors.fill: parent
        color: Theme.surfaceColor
        radius: 10
        border.width: 1
        border.color: Theme.borderColor

        // Drop shadow effect
        layer.enabled: true
        layer.effect: Item {
            Rectangle {
                anchors.fill: parent
                anchors.margins: -4
                color: Qt.rgba(0, 0, 0, 0.15)
                radius: 14
                z: -1
            }
        }
    }

    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: qsTr("Trim Geometry")
                font.pixelSize: 16
                font.bold: true
                color: Theme.textPrimaryColor
            }

            Button {
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                flat: true
                icon.source: "qrc:/opengeolab/resources/icons/close.svg"
                icon.color: Theme.textSecondaryColor
                onClicked: root.closeRequested()

                background: Rectangle {
                    color: parent.hovered ? Theme.errorColor : "transparent"
                    radius: 4
                    opacity: 0.2
                }
            }
        }

        // Separator
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.borderColor
        }

        // Description
        Label {
            Layout.fillWidth: true
            text: qsTr("Select geometry to trim and configure options:")
            color: Theme.textSecondaryColor
            wrapMode: Text.Wrap
            font.pixelSize: 12
        }

        // Geometry selector for the element to trim
        Pages.GeometrySelector {
            id: trimTargetSelector
            Layout.fillWidth: true
            label: qsTr("Geometry to Trim:")
            selectionType: 3 // Face by default
            placeholder: qsTr("Click to select target...")

            onSelectionChanged: (id, type) => {
                if (root.viewport) {
                    root.viewport.selectionMode = isSelecting ? type : 0;
                }
            }

            onIsSelectingChanged: {
                if (root.viewport) {
                    root.viewport.selectionMode = isSelecting ? selectionType : 0;
                }
            }
        }

        // Geometry selector for the trim tool (plane/surface)
        Pages.GeometrySelector {
            id: trimToolSelector
            Layout.fillWidth: true
            label: qsTr("Trim Tool (Plane/Surface):")
            selectionType: 3 // Face by default
            placeholder: qsTr("Click to select tool...")
            visible: modeCombo.currentIndex !== 0

            onSelectionChanged: (id, type) => {
                if (root.viewport && isSelecting) {
                    root.viewport.selectionMode = type;
                }
            }

            onIsSelectingChanged: {
                if (root.viewport) {
                    root.viewport.selectionMode = isSelecting ? selectionType : 0;
                }
            }
        }

        // Separator
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.borderColor
        }

        // Trim options
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 12
            rowSpacing: 8

            Label {
                text: qsTr("Trim Mode:")
                color: Theme.textPrimaryColor
                font.pixelSize: 13
            }

            ComboBox {
                id: modeCombo
                Layout.fillWidth: true
                model: [qsTr("Auto"), qsTr("By Plane"), qsTr("By Surface")]
                enabled: !OGL.BackendService.busy

                background: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 32
                    color: modeCombo.enabled ? Theme.surfaceColor : Theme.surfaceAltColor
                    border.width: 1
                    border.color: modeCombo.pressed ? Theme.accentColor : Theme.borderColor
                    radius: 4
                }
            }
        }

        CheckBox {
            id: keepOriginalCheck
            text: qsTr("Keep original geometry")
            checked: false
            enabled: !OGL.BackendService.busy
        }

        // Status area
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: OGL.BackendService.busy

            BusyIndicator {
                Layout.preferredWidth: 20
                Layout.preferredHeight: 20
                running: true
            }

            Label {
                Layout.fillWidth: true
                text: OGL.BackendService.message
                color: Theme.textSecondaryColor
                font.pixelSize: 12
                elide: Text.ElideRight
            }
        }

        // Buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: qsTr("Cancel")
                onClicked: root.closeRequested()

                background: Rectangle {
                    implicitWidth: 80
                    implicitHeight: 32
                    color: parent.hovered ? Theme.surfaceAltColor : Theme.surfaceColor
                    border.width: 1
                    border.color: Theme.borderColor
                    radius: 4
                }
            }

            Button {
                text: qsTr("Apply")
                enabled: !OGL.BackendService.busy && trimTargetSelector.selectedId > 0
                onClicked: {
                    const params = {
                        targetId: trimTargetSelector.selectedId,
                        targetType: trimTargetSelector.selectionType,
                        toolId: trimToolSelector.selectedId,
                        toolType: trimToolSelector.selectionType,
                        mode: modeCombo.currentText,
                        keepOriginal: keepOriginalCheck.checked
                    };
                    OGL.BackendService.request("Trim", params);
                }

                background: Rectangle {
                    implicitWidth: 80
                    implicitHeight: 32
                    color: parent.enabled ? Theme.accentColor : Theme.surfaceAltColor
                    border.width: 1
                    border.color: parent.enabled ? Theme.accentColor : Theme.borderColor
                    radius: 4
                }

                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? "white" : Theme.textSecondaryColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    // Handle viewport selection
    Connections {
        target: root.viewport

        function onSelectionChanged(id: int, type: int): void {
            if (trimTargetSelector.isSelecting) {
                trimTargetSelector.setSelection(id, getSelectionInfo(id, type));
            } else if (trimToolSelector.isSelecting) {
                trimToolSelector.setSelection(id, getSelectionInfo(id, type));
            }
        }
    }

    // Handle operation completion
    Connections {
        target: OGL.BackendService

        function onOperationFinished(moduleName: string, _result: var): void {
            if (moduleName === "Trim") {
                trimTargetSelector.clearSelection();
                trimToolSelector.clearSelection();
            }
        }
    }

    function getSelectionInfo(id: int, type: int): string {
        switch (type) {
        case 1:
            return qsTr("Vertex %1").arg(id);
        case 2:
            return qsTr("Edge %1").arg(id);
        case 3:
            return qsTr("Face %1").arg(id);
        case 4:
            return qsTr("Part %1").arg(id);
        default:
            return qsTr("Element %1").arg(id);
        }
    }

    function show() {
        root.isVisible = true;
    }

    function hide() {
        root.isVisible = false;
        trimTargetSelector.stopSelection();
        trimToolSelector.stopSelection();
        if (root.viewport) {
            root.viewport.selectionMode = 0;
        }
    }
}
