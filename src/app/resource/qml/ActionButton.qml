import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: root

    property string moduleName: ""
    property string actionName: ""
    property string label: "Action"
    property string payload: "{}"

    signal request(string requestJson)

    text: label
    enabled: !processService.busy

    Layout.fillWidth: true
    Layout.preferredHeight: 40

    onClicked: {
        let request = {
            action: root.actionName,
            requestId: "qml-" + Date.now(),
            payload: JSON.parse(root.payload)
        };
        if (root.moduleName !== "") {
            request.module = root.moduleName;
        }
        root.request(JSON.stringify(request));
    }
}
