pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * RibbonFileMenuV2.qml
 * --------------------
 * 目标：先做一个最小可用的 File Popup。
 * 后续你可以把旧版 RibbonFileMenu 的“左侧菜单 + 右侧 Recent”逐步加回来。
 */
Popup {
    id: root

    signal actionTriggered(string actionId, var payload)

    width: 360
    height: 260
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 0

    property var recentFiles: []

    background: Rectangle {
        color: Theme.ribbonContentColor
        border.color: Theme.ribbonBorderColor
        border.width: 1
    }

    contentItem: RowLayout {
        anchors.fill: parent
        spacing: 0

        // 左侧操作区
        Rectangle {
            Layout.preferredWidth: 160
            Layout.fillHeight: true
            color: Theme.ribbonFileMenuLeftColor

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                Button {
                    text: qsTr("New")
                    onClicked: {
                        root.close();
                        root.actionTriggered("newFile", null);
                    }
                }
                Button {
                    text: qsTr("Open")
                    onClicked: {
                        root.close();
                        root.actionTriggered("openFile", null);
                    }
                }
                Button {
                    text: qsTr("Import")
                    onClicked: {
                        root.close();
                        root.actionTriggered("importModel", null);
                    }
                }
                Button {
                    text: qsTr("Export")
                    onClicked: {
                        root.close();
                        root.actionTriggered("exportModel", null);
                    }
                }
                Item {
                    Layout.fillHeight: true
                }
                Button {
                    text: qsTr("Exit")
                    onClicked: {
                        root.close();
                        root.actionTriggered("exitApp", null);
                    }
                }
            }
        }

        // 右侧 Recent Files（最小版）
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.ribbonContentColor

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                Text {
                    text: qsTr("Recent Files")
                    color: Theme.ribbonTextColor
                    font.pixelSize: 12
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: root.recentFiles

                    delegate: ItemDelegate {
                        required property var modelData
                        width: ListView.view.width
                        text: modelData
                        onClicked: {
                            // payload 里把文件路径带出去，业务层决定怎么打开
                            root.close();
                            root.actionTriggered("openRecent", {
                                path: modelData
                            });
                        }
                    }
                }
            }
        }
    }

    function setRecentFiles(files): void {
        root.recentFiles = files || [];
    }
}
