import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    function appendResponse(text, isError, requestId) {
        messageModel.insert(0, {
            timestamp: new Date().toLocaleTimeString(),
            requestId: requestId || "",
            content: text,
            isError: isError || false,
            expanded: false
        });
        if (messageModel.count > 100)
            messageModel.remove(messageModel.count - 1);
    }

    Layout.fillWidth: true
    Layout.fillHeight: true

    ListModel { id: messageModel }

    ListView {
        id: messageList
        anchors.fill: parent
        model: messageModel
        spacing: 4
        clip: true

        delegate: Rectangle {
            width: ListView.view.width
            height: contentCol.implicitHeight + 16
            radius: 4
            color: model.isError ? "#FFF0F0" : "#F0F8F0"
            border.color: model.isError ? "#CC3333" : "#33AA33"
            border.width: 1

            ColumnLayout {
                id: contentCol
                anchors.fill: parent
                anchors.margins: 8
                spacing: 4

                RowLayout {
                    Label {
                        text: model.timestamp
                        font.pixelSize: 11
                        color: "#666666"
                    }
                    Label {
                        text: model.isError ? "✗ ERROR" : "✓ OK"
                        font.pixelSize: 11
                        font.bold: true
                        color: model.isError ? "#CC3333" : "#33AA33"
                    }
                    Label {
                        text: model.requestId
                        font.pixelSize: 11
                        color: "#336699"
                        visible: model.requestId !== ""
                        elide: Text.ElideMiddle
                        Layout.maximumWidth: 140
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: model.expanded ? "▲ 收起" : "▼ 展开"
                        font.pixelSize: 11
                        color: "#0066CC"
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: messageModel.setProperty(index, "expanded", !model.expanded)
                        }
                    }
                }

                Label {
                    text: model.expanded ? model.content : model.content.substring(0, 120) + (model.content.length > 120 ? "..." : "")
                    wrapMode: Text.Wrap
                    font.family: "Consolas"
                    font.pixelSize: 12
                    Layout.fillWidth: true
                    maximumLineCount: model.expanded ? -1 : 3
                    elide: model.expanded ? Text.ElideNone : Text.ElideRight
                }
            }
        }
    }

    Label {
        anchors.centerIn: parent
        text: "等待响应..."
        color: "#999999"
        visible: messageModel.count === 0
    }
}
