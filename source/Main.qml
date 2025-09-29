import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 800
    height: 600
    visible: true
    title: qsTr("OpenGeoLab")

    // 主要内容区域
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // 标题区域
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            color: "#f0f0f0"
            radius: 8

            Text {
                anchors.centerIn: parent
                text: qsTr("欢迎使用 OpenGeoLab")
                font.pixelSize: 24
                font.bold: true
                color: "#333333"
            }
        }

        // 内容区域
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                width: parent.width
                spacing: 15

                // 信息卡片
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("项目信息")

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 10

                        Text {
                            text: qsTr("这是一个基于 Qt Quick 的 OpenGeoLab 应用程序")
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        Text {
                            text: qsTr("版本: 1.0.0")
                            color: "#666666"
                        }

                        Text {
                            text: qsTr("构建时间: ") + new Date().toLocaleString()
                            color: "#666666"
                        }
                    }
                }

                // 功能按钮区域
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("功能选项")

                    GridLayout {
                        anchors.fill: parent
                        columns: 2
                        columnSpacing: 15
                        rowSpacing: 10

                        Button {
                            text: qsTr("新建项目")
                            Layout.fillWidth: true
                            onClicked: {
                                console.log("新建项目被点击");
                            }
                        }

                        Button {
                            text: qsTr("打开项目")
                            Layout.fillWidth: true
                            onClicked: {
                                console.log("打开项目被点击");
                            }
                        }

                        Button {
                            text: qsTr("设置")
                            Layout.fillWidth: true
                            onClicked: {
                                console.log("设置被点击");
                            }
                        }

                        Button {
                            text: qsTr("帮助")
                            Layout.fillWidth: true
                            onClicked: {
                                console.log("帮助被点击");
                            }
                        }
                    }
                }

                // 状态区域
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    color: "#e8f5e8"
                    radius: 4
                    border.color: "#4caf50"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Rectangle {
                            Layout.preferredWidth: 12
                            Layout.preferredHeight: 12
                            radius: 6
                            color: "#4caf50"
                        }

                        Text {
                            text: qsTr("应用程序运行正常")
                            color: "#2e7d32"
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }
    }

    // 状态栏
    footer: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.margins: 5

            Label {
                text: qsTr("就绪")
                Layout.fillWidth: true
            }

            Label {
                text: Qt.formatDateTime(new Date(), "yyyy-MM-dd hh:mm:ss")
                color: "#666666"

                Timer {
                    interval: 1000
                    running: true
                    repeat: true
                    onTriggered: parent.text = Qt.formatDateTime(new Date(), "yyyy-MM-dd hh:mm:ss")
                }
            }
        }
    }
}
