pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * @brief Model Tree View - 模型层次结构树视图
 *
 * 显示当前模型的层次结构，方便用户进行模型管理和操作
 */
Rectangle {
    id: modelTreeView

    property color headerColor: "#2B579A"
    property color backgroundColor: Qt.rgba(0.15, 0.15, 0.15, 0.95)
    property color itemHoverColor: Qt.rgba(0.3, 0.3, 0.3, 0.8)
    property color selectedColor: "#0078D4"
    property color textColor: "white"

    // 当前选中的项目
    property int selectedIndex: -1

    // 信号
    signal itemSelected(int index, string name, string type)
    signal itemDoubleClicked(int index, string name, string type)

    color: backgroundColor

    // 树节点模型
    ListModel {
        id: treeModel

        // 默认结构
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

        // 标题栏
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

                // 刷新按钮
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

                // 折叠全部按钮
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
                            // 折叠所有节点
                            for (let i = 0; i < treeModel.count; i++) {
                                treeModel.setProperty(i, "expanded", false);
                            }
                        }
                    }
                }
            }
        }

        // 搜索框
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

        // 树视图列表
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

                    // 展开/折叠图标
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

                    // 占位符（当没有子节点时）
                    Item {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        visible: !treeItemDelegate.hasChildren
                    }

                    // 节点图标
                    Text {
                        text: modelTreeView.getNodeIcon(treeItemDelegate.nodeType)
                        font.pixelSize: 14
                        color: modelTreeView.getIconColor(treeItemDelegate.nodeType)
                    }

                    // 节点名称
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
                            // 右键菜单（TODO）
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

        // 底部工具栏
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

                // 添加按钮
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

                // 删除按钮
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

    // 根据节点类型获取图标
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

    // 根据节点类型获取图标颜色
    function getIconColor(type: string): color {
        switch (type) {
        case "root":
            return "#FFD700";  // 金色
        case "folder":
            return "#FFA500";  // 橙色
        case "body":
            return "#4FC3F7";  // 浅蓝色
        case "surface":
            return "#81C784";  // 浅绿色
        case "curve":
            return "#BA68C8";  // 紫色
        case "point":
            return "#FF8A65";  // 橙红色
        case "mesh":
            return "#64B5F6";  // 蓝色
        case "material":
            return "#FFB74D";  // 橙黄色
        default:
            return "white";
        }
    }

    // 公共函数：添加节点
    function addNode(name: string, type: string, parentLevel: int): void {
        treeModel.append({
            name: name,
            nodeType: type,
            level: parentLevel + 1,
            expanded: false,
            hasChildren: false
        });
    }

    // 公共函数：清空模型树
    function clearTree(): void {
        treeModel.clear();
    }

    // 公共函数：重置为默认结构
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
