pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * @file ModelTreeView.qml
 * @brief Model Tree View - Hierarchical model structure display
 *
 * Displays the hierarchical structure of the current model for
 * model management and navigation operations.
 */
Rectangle {
    id: modelTreeView

    // ========================================================================
    // Dark theme colors (fixed)
    // ========================================================================
    readonly property color headerColor: "#1a1d23"
    readonly property color backgroundColor: "#252830"
    readonly property color itemHoverColor: "#3a3f4b"
    readonly property color selectedColor: "#0d6efd"
    readonly property color textColor: "#e1e1e1"
    readonly property color borderColor: "#363b44"
    readonly property color iconColor: "#e1e1e1"

    // Currently selected item
    property int selectedIndex: -1

    property int vertices: 0
    property int triangles: 0

    property string importedFilename: ""
    property var importedParts: []

    // Signals
    signal itemSelected(int index, string name, string type)
    signal itemDoubleClicked(int index, string name, string type)

    color: backgroundColor

    onImportedFilenameChanged: {
        if (importedFilename && importedFilename.length > 0) {
            modelTreeView.setImportedModel(importedFilename, modelTreeView.vertices, modelTreeView.triangles, modelTreeView.importedParts);
        }
    }

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
            parentIndex: -1
            nodeVisible: true
            subtitle: ""
            badgeText: ""
        }
        ListElement {
            name: "Geometry"
            nodeType: "folder"
            level: 1
            expanded: true
            hasChildren: true
            parentIndex: 0
            nodeVisible: true
            subtitle: ""
            badgeText: ""
        }
        ListElement {
            name: "Bodies"
            nodeType: "folder"
            level: 2
            expanded: false
            hasChildren: true
            parentIndex: 1
            nodeVisible: true
            subtitle: ""
            badgeText: ""
        }
        ListElement {
            name: "Surfaces"
            nodeType: "folder"
            level: 2
            expanded: false
            hasChildren: false
            parentIndex: 1
            nodeVisible: true
            subtitle: ""
            badgeText: ""
        }
        ListElement {
            name: "Curves"
            nodeType: "folder"
            level: 2
            expanded: false
            hasChildren: false
            parentIndex: 1
            nodeVisible: true
            subtitle: ""
            badgeText: ""
        }
        ListElement {
            name: "Points"
            nodeType: "folder"
            level: 2
            expanded: false
            hasChildren: false
            parentIndex: 1
            nodeVisible: true
            subtitle: ""
            badgeText: ""
        }
        ListElement {
            name: "Mesh"
            nodeType: "folder"
            level: 1
            expanded: false
            hasChildren: true
            parentIndex: 0
            nodeVisible: true
            subtitle: ""
            badgeText: ""
        }
        ListElement {
            name: "Materials"
            nodeType: "folder"
            level: 1
            expanded: false
            hasChildren: false
            parentIndex: 0
            nodeVisible: true
            subtitle: ""
            badgeText: ""
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
                    text: "Part Tree"
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
                            modelTreeView.recomputeVisibility();
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
                required property int parentIndex
                required property bool nodeVisible
                required property string subtitle
                required property string badgeText

                width: treeListView.width
                height: treeItemDelegate.nodeVisible ? 34 : 0
                opacity: treeItemDelegate.nodeVisible ? 1 : 0
                visible: treeItemDelegate.nodeVisible
                color: modelTreeView.selectedIndex === index ? modelTreeView.selectedColor : (itemMouseArea.containsMouse ? modelTreeView.itemHoverColor : "transparent")
                radius: 3
                clip: true

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
                                modelTreeView.recomputeVisibility();
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
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        Text {
                            text: treeItemDelegate.name
                            font.pixelSize: 12
                            font.bold: treeItemDelegate.nodeType === "root" || treeItemDelegate.nodeType === "folder"
                            color: modelTreeView.textColor
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            visible: treeItemDelegate.subtitle.length > 0
                            text: treeItemDelegate.subtitle
                            font.pixelSize: 10
                            color: Qt.rgba(modelTreeView.textColor.r, modelTreeView.textColor.g, modelTreeView.textColor.b, 0.7)
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    // Badge (right aligned)
                    Rectangle {
                        visible: treeItemDelegate.badgeText.length > 0
                        Layout.preferredHeight: 18
                        Layout.preferredWidth: Math.max(34, badgeLabel.implicitWidth + 10)
                        radius: 9
                        color: Qt.rgba(1, 1, 1, 0.08)
                        border.width: 1
                        border.color: modelTreeView.borderColor

                        Text {
                            id: badgeLabel
                            anchors.centerIn: parent
                            text: treeItemDelegate.badgeText
                            font.pixelSize: 10
                            color: modelTreeView.textColor
                        }
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
                            modelTreeView.recomputeVisibility();
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
                    text: "Items: " + modelTreeView.visibleItemCount() + " / " + treeModel.count
                    font.pixelSize: 11
                    color: Qt.rgba(0.6, 0.6, 0.6, 1)
                }

                Item {
                    Layout.fillWidth: true
                }

                Text {
                    text: modelTreeView.vertices > 0 ? ("V: " + modelTreeView.vertices + "  T: " + modelTreeView.triangles) : ""
                    font.pixelSize: 11
                    color: Qt.rgba(0.6, 0.6, 0.6, 1)
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
        case "part":
            return "🧩";
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
        case "part":
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
            hasChildren: false,
            parentIndex: -1,
            nodeVisible: true,
            subtitle: "",
            badgeText: ""
        });
        modelTreeView.recomputeVisibility();
    }

    // Public function: Clear model tree
    function clearTree(): void {
        treeModel.clear();
        modelTreeView.selectedIndex = -1;
    }

    function visibleItemCount(): int {
        let c = 0;
        for (let i = 0; i < treeModel.count; i++) {
            if (treeModel.get(i).nodeVisible)
                c++;
        }
        return c;
    }

    function recomputeVisibility(): void {
        for (let i = 0; i < treeModel.count; i++) {
            const node = treeModel.get(i);
            if (node.parentIndex === -1) {
                treeModel.setProperty(i, "nodeVisible", true);
                continue;
            }

            const parent = treeModel.get(node.parentIndex);
            const parentVisible = parent.nodeVisible === true;
            const parentExpanded = parent.expanded === true;
            treeModel.setProperty(i, "nodeVisible", parentVisible && parentExpanded);
        }
    }

    // Public function: Update tree using imported model parts
    function setImportedModel(filename: string, vertices: int, triangles: int, parts: var): void {
        modelTreeView.vertices = vertices;
        modelTreeView.triangles = triangles;

        clearTree();

        // Root
        treeModel.append({
            name: filename && filename.length > 0 ? filename : "Model",
            nodeType: "root",
            level: 0,
            expanded: true,
            hasChildren: true,
            parentIndex: -1,
            nodeVisible: true,
            subtitle: vertices > 0 ? ("Vertices: " + vertices + "  Triangles: " + triangles) : "",
            badgeText: ""
        });

        // Parts folder
        const partsFolderIndex = treeModel.count;
        const partCount = parts ? parts.length : 0;
        treeModel.append({
            name: "Parts",
            nodeType: "folder",
            level: 1,
            expanded: true,
            hasChildren: partCount > 0,
            parentIndex: 0,
            nodeVisible: true,
            subtitle: partCount > 0 ? (partCount + " items") : "No parts",
            badgeText: partCount > 0 ? ("" + partCount) : ""
        });

        // Part nodes
        if (parts && parts.length) {
            for (let i = 0; i < parts.length; i++) {
                const p = parts[i];
                const faceCount = p.faceCount !== undefined ? p.faceCount : 0;
                const edgeCount = p.edgeCount !== undefined ? p.edgeCount : 0;
                const subtitle = "Faces: " + faceCount + "  Edges: " + edgeCount;

                treeModel.append({
                    name: p.name ? p.name : ("Part " + (i + 1)),
                    nodeType: "part",
                    level: 2,
                    expanded: false,
                    hasChildren: false,
                    parentIndex: partsFolderIndex,
                    nodeVisible: true,
                    subtitle: subtitle,
                    badgeText: faceCount > 0 ? ("F " + faceCount) : ""
                });
            }
        }

        recomputeVisibility();
    }

    // Public function: Reset to default structure
    function resetToDefault(): void {
        clearTree();
        treeModel.append({
            name: "Model",
            nodeType: "root",
            level: 0,
            expanded: true,
            hasChildren: true,
            parentIndex: -1,
            nodeVisible: true,
            subtitle: "",
            badgeText: ""
        });
        treeModel.append({
            name: "Geometry",
            nodeType: "folder",
            level: 1,
            expanded: true,
            hasChildren: true,
            parentIndex: 0,
            nodeVisible: true,
            subtitle: "",
            badgeText: ""
        });
        treeModel.append({
            name: "Bodies",
            nodeType: "folder",
            level: 2,
            expanded: false,
            hasChildren: true,
            parentIndex: 1,
            nodeVisible: true,
            subtitle: "",
            badgeText: ""
        });
        treeModel.append({
            name: "Surfaces",
            nodeType: "folder",
            level: 2,
            expanded: false,
            hasChildren: false,
            parentIndex: 1,
            nodeVisible: true,
            subtitle: "",
            badgeText: ""
        });
        treeModel.append({
            name: "Curves",
            nodeType: "folder",
            level: 2,
            expanded: false,
            hasChildren: false,
            parentIndex: 1,
            nodeVisible: true,
            subtitle: "",
            badgeText: ""
        });
        treeModel.append({
            name: "Points",
            nodeType: "folder",
            level: 2,
            expanded: false,
            hasChildren: false,
            parentIndex: 1,
            nodeVisible: true,
            subtitle: "",
            badgeText: ""
        });
        treeModel.append({
            name: "Mesh",
            nodeType: "folder",
            level: 1,
            expanded: false,
            hasChildren: true,
            parentIndex: 0,
            nodeVisible: true,
            subtitle: "",
            badgeText: ""
        });
        treeModel.append({
            name: "Materials",
            nodeType: "folder",
            level: 1,
            expanded: false,
            hasChildren: false,
            parentIndex: 0,
            nodeVisible: true,
            subtitle: "",
            badgeText: ""
        });

        modelTreeView.vertices = 0;
        modelTreeView.triangles = 0;
        recomputeVisibility();
    }
}
