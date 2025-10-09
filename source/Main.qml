pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import OpenGeoLab

Window {
    id: root
    visible: true
    width: 960
    height: 600
    title: "OpenGeoLab - Triangle Demo"

    Squircle {
        // 设置 Squircle 的位置和大小,避开左侧控制面板
        anchors.left: controlPanel.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        SequentialAnimation on t {
            NumberAnimation {
                to: 1
                duration: 2500
                easing.type: Easing.InQuad
            }
            NumberAnimation {
                to: 0
                duration: 2500
                easing.type: Easing.OutQuad
            }
            loops: Animation.Infinite
            running: true
        }
    }

    // 左侧控制面板
    Rectangle {
        id: controlPanel
        width: 200
        height: parent.height
        anchors.left: parent.left
        anchors.top: parent.top
        color: Qt.rgba(0.2, 0.2, 0.2, 0.9)

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 15
            spacing: 20

            // 标题
            Text {
                text: "几何体控制面板"
                color: "white"
                font.pixelSize: 16
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            // 分隔线
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 2
                color: Qt.rgba(1, 1, 1, 0.3)
            }

            // 几何体选择区域
            Text {
                text: "选择几何体:"
                color: "white"
                font.pixelSize: 14
            }

            Button {
                text: "立方体"
                Layout.fillWidth: true
                onClicked:
                // TODO: 切换到立方体
                {}
            }

            Button {
                text: "圆柱体"
                Layout.fillWidth: true
                onClicked:
                // TODO: 切换到圆柱体
                {}
            }

            Button {
                text: "球体"
                Layout.fillWidth: true
                onClicked:
                // TODO: 切换到球体
                {}
            }

            Button {
                text: "圆锥体"
                Layout.fillWidth: true
                onClicked:
                // TODO: 切换到圆锥体
                {}
            }

            // 分隔线
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 2
                color: Qt.rgba(1, 1, 1, 0.3)
                Layout.topMargin: 10
            }

            // 颜色选择区域
            Text {
                text: "选择颜色:"
                color: "white"
                font.pixelSize: 14
            }

            Button {
                text: "红色"
                Layout.fillWidth: true
                onClicked:
                // TODO: 设置红色
                {}
            }

            Button {
                text: "绿色"
                Layout.fillWidth: true
                onClicked:
                // TODO: 设置绿色
                {}
            }

            Button {
                text: "蓝色"
                Layout.fillWidth: true
                onClicked:
                // TODO: 设置蓝色
                {}
            }

            Button {
                text: "黄色"
                Layout.fillWidth: true
                onClicked:
                // TODO: 设置黄色
                {}
            }

            // 占位符,填充剩余空间
            Item {
                Layout.fillHeight: true
            }
        }
    }

    Rectangle {
        color: Qt.rgba(1, 1, 1, 0.7)
        radius: 10
        border.width: 1
        border.color: "white"
        anchors.fill: label
        anchors.margins: -10
    }

    Text {
        id: label
        color: "black"
        wrapMode: Text.WordWrap
        text: qsTr("The background here is a squircle rendered with raw OpenGL using the 'beforeRender()' signal in QQuickWindow. This text label and its border is rendered using QML")
        anchors.right: parent.right
        anchors.left: controlPanel.right
        anchors.leftMargin: 20
        anchors.bottom: parent.bottom
        anchors.margins: 20
    }
}
