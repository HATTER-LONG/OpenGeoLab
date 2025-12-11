pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * @brief Model Tree View - Hierarchical model structure tree view
 *
 * Displays the hierarchical structure of the current model for model management and operations.
 * Theme colors can be overridden externally.
 */
Rectangle {
    id: modelTreeView

    // Theme mode: true = dark, false = light
    property bool isDarkTheme: true

    // Theme colors - automatically switch based on theme
    property color headerColor: isDarkTheme ? "#1a1d23" : "#f0f0f0"
    property color backgroundColor: isDarkTheme ? "#252830" : "#ffffff"
    property color itemHoverColor: isDarkTheme ? "#3a3f4b" : "#e5f1fb"
    property color selectedColor: isDarkTheme ? "#0d6efd" : "#0078d4"
    property color textColor: isDarkTheme ? "#e1e1e1" : "#1a1a1a"
    property color borderColor: isDarkTheme ? "#363b44" : "#d1d1d1"
    property color iconColor: isDarkTheme ? "#e1e1e1" : "#333333"

    // Currently selected item
    property int selectedIndex: -1

    // Signals
    signal itemSelected(int index, string name, string type)
    signal itemDoubleClicked(int index, string name, string type)

    color: backgroundColor

    // Right border for visual separation
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: modelTreeView.borderColor
    }

    // Tree node model
    ListModel {
        id: treeModel

        // Default structure
        ListElement {
            name: "Model"
            nodeType: "root"
            level: 0
            expanded: true
            hasChildren: true
        }
        ListElement {
            name: "Geometry"
            nodeType: "folder"
            level: 1
            expanded: true
            hasChildren: true
        }
        ListElement {
            name: "Bodies"
            nodeType: "folder"
            level: 2
            expanded: false
            hasChildren: true
        }
        ListElement {
            name: "Surfaces"
            nodeType: "folder"
            level: 2
            expanded: false
            hasChildren: false
        }
        ListElement {
            name: "Curves"
            nodeType: "folder"
            level: 2
            expanded: false
            hasChildren: false
        }
        ListElement {
            name: "Points"
            nodeType: "folder"
            level: 2
            expanded: false
            hasChildren: false
        }
        ListElement {
            name: "Mesh"
            nodeType: "folder"
            level: 1
            expanded: false
            hasChildren: true
        }
        ListElement {
            name: "Materials"
            nodeType: "folder"
            level: 1
            expanded: false
            hasChildren: false
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            color: modelTreeView.headerColor

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 5
                spacing: 8

                Text {
                    text: "🗂"
                    font.pixelSize: 14
                    color: "white"
                }

                Text {
                    text: "Model Tree"
                    font.pixelSize: 13
                    font.bold: true
                    color: "white"
                    Layout.fillWidth: true
                }

                // Refresh button
                Rectangle {
                    Layout.preferredWidth: 22
                    Layout.preferredHeight: 22
                    color: refreshArea.containsMouse ? Qt.rgba(1, 1, 1, 0.2) : "transparent"
                    radius: 3

                    Text {
                        anchors.centerIn: parent
                        text: "🔄"
                        font.pixelSize: 12
                    }

                    MouseArea {
                        id: refreshArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            console.log("Refresh model tree");
                        }
                    }
                }

                // Collapse all button
                Rectangle {
                    Layout.preferredWidth: 22
                    Layout.preferredHeight: 22
                    color: collapseArea.containsMouse ? Qt.rgba(1, 1, 1, 0.2) : "transparent"
                    radius: 3

                    Text {
                        anchors.centerIn: parent
                        text: "▼"
                        font.pixelSize: 10
                        color: "white"
                    }

                    MouseArea {
                        id: collapseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            // Collapse all nodes
                            for (let i = 0; i < treeModel.count; i++) {
                                treeModel.setProperty(i, "expanded", false);
                            }
                        }
                    }
                }
            }
        }

        // Search box
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            Layout.margins: 5
            color: Qt.rgba(0.2, 0.2, 0.2, 1)
            radius: 3
            border.color: searchField.activeFocus ? modelTreeView.selectedColor : Qt.rgba(0.4, 0.4, 0.4, 1)
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 5

                Text {
                    text: "🔍"
                    font.pixelSize: 12
                    color: Qt.rgba(0.6, 0.6, 0.6, 1)
                }

                TextInput {
                    id: searchField
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    verticalAlignment: TextInput.AlignVCenter
                    color: modelTreeView.textColor
                    font.pixelSize: 12
                    clip: true

                    Text {
                        anchors.fill: parent
                        verticalAlignment: Text.AlignVCenter
                        text: "Search..."
                        color: Qt.rgba(0.5, 0.5, 0.5, 1)
                        font.pixelSize: 12
                        visible: !searchField.text && !searchField.activeFocus
                    }
                }
            }
        }

        // Tree view list
        ListView {
            id: treeListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 5
            clip: true
            spacing: 1

            model: treeModel

            delegate: Rectangle {
                id: treeItemDelegate
                required property int index
                required property string name
                required property string nodeType
                required property int level
                required property bool expanded
                required property bool hasChildren

                width: treeListView.width
                height: 26
                color: modelTreeView.selectedIndex === index ? modelTreeView.selectedColor : (itemMouseArea.containsMouse ? modelTreeView.itemHoverColor : "transparent")
                radius: 3

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8 + treeItemDelegate.level * 16
                    anchors.rightMargin: 8
                    spacing: 6

                    // Expand/collapse icon
                    Rectangle {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        color: "transparent"
                        visible: treeItemDelegate.hasChildren

                        Text {
                            anchors.centerIn: parent
                            text: treeItemDelegate.expanded ? "▼" : "▶"
                            font.pixelSize: 8
                            color: modelTreeView.textColor
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                treeModel.setProperty(treeItemDelegate.index, "expanded", !treeItemDelegate.expanded);
                            }
                        }
                    }

                    // Placeholder (when no children)
                    Item {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        visible: !treeItemDelegate.hasChildren
                    }

                    // Node icon
                    Text {
                        text: modelTreeView.getNodeIcon(treeItemDelegate.nodeType)
                        font.pixelSize: 14
                        color: modelTreeView.getIconColor(treeItemDelegate.nodeType)
                    }

                    // Node name
                    Text {
                        text: treeItemDelegate.name
                        font.pixelSize: 12
                        color: modelTreeView.textColor
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                MouseArea {
                    id: itemMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton

                    onClicked: mouse => {
                        modelTreeView.selectedIndex = treeItemDelegate.index;
                        if (mouse.button === Qt.LeftButton) {
                            modelTreeView.itemSelected(treeItemDelegate.index, treeItemDelegate.name, treeItemDelegate.nodeType);
                        } else if (mouse.button === Qt.RightButton) {
                            // Context menu (TODO)
                            console.log("Right click on:", treeItemDelegate.name);
                        }
                    }

                    onDoubleClicked: {
                        modelTreeView.itemDoubleClicked(treeItemDelegate.index, treeItemDelegate.name, treeItemDelegate.nodeType);
                        if (treeItemDelegate.hasChildren) {
                            treeModel.setProperty(treeItemDelegate.index, "expanded", !treeItemDelegate.expanded);
                        }
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }
        }

        // Bottom toolbar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: Qt.rgba(0.1, 0.1, 0.1, 1)

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 5

                Text {
                    text: "Items: " + treeModel.count
                    font.pixelSize: 11
                    color: Qt.rgba(0.6, 0.6, 0.6, 1)
                }

                Item {
                    Layout.fillWidth: true
                }

                // Add button
                Rectangle {
                    Layout.preferredWidth: 22
                    Layout.preferredHeight: 22
                    color: addArea.containsMouse ? Qt.rgba(0.3, 0.3, 0.3, 1) : "transparent"
                    radius: 3

                    Text {
                        anchors.centerIn: parent
                        text: "+"
                        font.pixelSize: 14
                        font.bold: true
                        color: modelTreeView.textColor
                    }

                    MouseArea {
                        id: addArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            console.log("Add new item");
                        }
                    }
                }

                // Delete button
                Rectangle {
                    Layout.preferredWidth: 22
                    Layout.preferredHeight: 22
                    color: deleteArea.containsMouse ? Qt.rgba(0.5, 0.2, 0.2, 1) : "transparent"
                    radius: 3

                    Text {
                        anchors.centerIn: parent
                        text: "🗑"
                        font.pixelSize: 12
                    }

                    MouseArea {
                        id: deleteArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (modelTreeView.selectedIndex >= 0) {
                                console.log("Delete selected item");
                            }
                        }
                    }
                }
            }
        }
    }

    // Get icon based on node type
    function getNodeIcon(type: string): string {
        switch (type) {
        case "root":
            return "📦";
        case "folder":
            return "📁";
        case "body":
            return "🧊";
        case "surface":
            return "◻";
        case "curve":
            return "〰";
        case "point":
            return "•";
        case "mesh":
            return "🔷";
        case "material":
            return "🎨";
        default:
            return "📄";
        }
    }

    // Get icon color based on node type
    function getIconColor(type: string): color {
        switch (type) {
        case "root":
            return "#FFD700";  // Gold
        case "folder":
            return "#FFA500";  // Orange
        case "body":
            return "#4FC3F7";  // Light blue
        case "surface":
            return "#81C784";  // Light green
        case "curve":
            return "#BA68C8";  // Purple
        case "point":
            return "#FF8A65";  // Orange-red
        case "mesh":
            return "#64B5F6";  // Blue
        case "material":
            return "#FFB74D";  // Orange-yellow
        default:
            return "white";
        }
    }

    // Public function: Add node
    function addNode(name: string, type: string, parentLevel: int): void {
        treeModel.append({
            name: name,
            nodeType: type,
            level: parentLevel + 1,
            expanded: false,
            hasChildren: false
        });
    }

    // Public function: Clear model tree
    function clearTree(): void {
        treeModel.clear();
    }

    // Public function: Reset to default structure
    function resetToDefault(): void {
        clearTree();
        treeModel.append({
            name: "Model",
            nodeType: "root",
            level: 0,
            expanded: true,
            hasChildren: true
        });
        treeModel.append({
            name: "Geometry",
            nodeType: "folder",
            level: 1,
            expanded: true,
            hasChildren: true
        });
        treeModel.append({
            name: "Bodies",
            nodeType: "folder",
            level: 2,
            expanded: false,
            hasChildren: true
        });
        treeModel.append({
            name: "Surfaces",
            nodeType: "folder",
            level: 2,
            expanded: false,
            hasChildren: false
        });
        treeModel.append({
            name: "Curves",
            nodeType: "folder",
            level: 2,
            expanded: false,
            hasChildren: false
        });
        treeModel.append({
            name: "Points",
            nodeType: "folder",
            level: 2,
            expanded: false,
            hasChildren: false
        });
        treeModel.append({
            name: "Mesh",
            nodeType: "folder",
            level: 1,
            expanded: false,
            hasChildren: true
        });
        treeModel.append({
            name: "Materials",
            nodeType: "folder",
            level: 1,
            expanded: false,
            hasChildren: false
        });
    }
}
