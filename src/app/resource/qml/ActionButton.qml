import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: root

    property string actionName: ""
    property string label: "Action"
    property string payload: "{}"

    signal request(string requestJson)

    text: label
    enabled: !processService.busy

    Layout.fillWidth: true
    Layout.preferredHeight: 40

    onClicked: {
        const requestJson = JSON.stringify({
            action: root.actionName,
            requestId: "qml-" + Date.now(),
            payload: JSON.parse(root.payload)
        })
        root.request(requestJson)
    }
}
