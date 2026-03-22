import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: root

    property string responseText: "等待响应...\n"

    function appendResponse(text) {
        const timestamp = new Date().toLocaleTimeString()
        responseText += "\n[" + timestamp + "]\n" + text + "\n"
        // Auto-scroll to bottom on next frame.
        Qt.callLater(() => {
            if (contentItem)
                contentItem.contentY = Math.max(0, contentItem.contentHeight - height)
        })
    }

    Layout.fillWidth: true
    Layout.fillHeight: true

    TextArea {
        text: root.responseText
        readOnly: true
        wrapMode: TextArea.Wrap
        font.family: "Consolas"
        font.pixelSize: 12
        selectByMouse: true
    }
}
