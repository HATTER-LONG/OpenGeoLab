pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property var groups: []

    property color iconColor: Theme.ribbonIconColor
    property color textColor: Theme.ribbonTextColor
    property color textColorDim: Theme.ribbonTextDimColor
    property color hoverColor: Theme.ribbonHoverColor
    property color pressedColor: Theme.ribbonPressedColor
    property color separatorColor: Theme.ribbonBorderColor

    signal actionTriggered(string actionId, var params)
    Row {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 6

        Repeater {
            model: root.groups

            Rectangle {
                id: groupBox
                required property var modelData

                // groupBox 本身透明，仅做布局容器
                color: "transparent"
                height: parent ? parent.height : 90
                implicitWidth: contentRow.implicitWidth + 14

                Column {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 4

                    // 上方按钮行
                    Row {
                        id: contentRow
                        spacing: 4

                        Repeater {
                            model: (groupBox.modelData.items || [])

                            Loader {
                                required property var modelData

                                // 根据 type 选择组件
                                sourceComponent: {
                                    const t = (modelData.type || "button");
                                    if (t === "separator")
                                        return separatorComponent;
                                    return buttonComponent;
                                }

                                onLoaded: {
                                    // 把 modelData 注入到实例里（按钮/分隔线）
                                    if (item && item.hasOwnProperty("itemData")) {
                                        item.itemData = modelData;
                                    }
                                }
                            }
                        }
                    }

                    // 底部组标题
                    Text {
                        text: groupBox.modelData.title || ""
                        color: root.textColorDim
                        font.pixelSize: 10
                        horizontalAlignment: Text.AlignHCenter
                        width: contentRow.width
                        elide: Text.ElideRight
                    }
                }

                // 组右侧竖线分隔
                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    width: 1
                    color: root.separatorColor
                    opacity: 0.9
                }
            }
        }
    }

    // ===== 按钮模板 =====
    Component {
        id: buttonComponent

        RibbonLargeButton {
            id: btn

            // Loader 会注入
            property var itemData: ({})
            iconColor: root.iconColor
            textColor: root.textColor
            hoverColor: root.hoverColor
            pressedColor: root.pressedColor

            // 容错：字段缺失也不崩
            iconSource: itemData.iconSource || ""
            text: itemData.text || qsTr("Unnamed")
            tooltipText: itemData.tooltip || ""

            onClicked: {
                // 统一向外发动作（payload 可用于扩展参数）
                root.actionTriggered(itemData.id || "", itemData.payload);
            }
        }
    }

    // ===== 分隔线模板 =====
    Component {
        id: separatorComponent

        Rectangle {
            // 分隔线不需要 itemData，但为了 Loader 结构一致可留着
            property var itemData: ({})
            width: 1
            height: 60
            color: root.separatorColor
            opacity: 0.9
        }
    }
}
