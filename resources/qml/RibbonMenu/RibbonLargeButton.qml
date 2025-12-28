pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

AbstractButton {
    id: root

    property string iconSource: ""
    property string tooltipText: ""

    // 主题可注入
    property color iconColor: Theme.ribbonIconColor
    property color textColor: Theme.ribbonTextColor
    property color hoverColor: Theme.ribbonHoverColor
    property color pressedColor: Theme.ribbonPressedColor

    // 统一尺寸（也可改成 Layout.preferredXXX 以适应容器）
    implicitWidth: 52
    implicitHeight: 64

    hoverEnabled: true
    focusPolicy: Qt.TabFocus

    background: Rectangle {
        radius: 3
        border.width: root.hovered || root.activeFocus ? 1 : 0
        border.color: root.hoverColor

        color: root.pressed ? root.pressedColor : (root.hovered ? root.hoverColor : "transparent")
    }

    contentItem: Column {
        anchors.centerIn: parent
        spacing: 2

        Item {
            width: 28
            height: 28
            anchors.horizontalCenter: parent.horizontalCenter

            // 原图隐藏，用 ColorOverlay 着色
            Image {
                id: iconImage
                anchors.fill: parent
                source: root.iconSource
                fillMode: Image.PreserveAspectFit
                visible: false
                smooth: true
                antialiasing: true
            }

            ColorOverlay {
                anchors.fill: iconImage
                source: iconImage
                color: root.iconColor
                visible: iconImage.status === Image.Ready
            }

            // 图标加载失败时显示 fallback（取 text 首字母）
            Text {
                anchors.centerIn: parent
                visible: iconImage.status !== Image.Ready
                text: (root.text && root.text.length > 0) ? root.text[0] : "?"
                color: root.textColor
                font.pixelSize: 14
            }
        }

        Text {
            text: root.text
            color: root.textColor
            font.pixelSize: 10
            horizontalAlignment: Text.AlignHCenter
            width: root.width - 6
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }
    }

    ToolTip.visible: root.hovered && root.tooltipText !== ""
    ToolTip.text: root.tooltipText
    ToolTip.delay: 500

    // 可访问性（可选但建议）
    Accessible.name: root.text
    Accessible.description: root.tooltipText
}
