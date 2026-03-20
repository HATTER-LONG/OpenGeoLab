import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: mainWindow

    width: 800
    height: 600
    visible: true
    title: "OpenGeoLab Pipeline Test"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Label {
            text: "OpenGeoLab Pipeline Test"
            font.pixelSize: 20
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        GridLayout {
            columns: 2
            Layout.fillWidth: true
            columnSpacing: 12
            rowSpacing: 8

            ActionButton {
                label: "Python 能力检查"
                actionName: "capabilities.query"
                onRequest: (json) => processService.submitRequest(json)
            }

            ActionButton {
                label: "列出插件"
                actionName: "plugins.list"
                onRequest: (json) => processService.submitRequest(json)
            }

            ActionButton {
                label: "启动 PySide6 窗口"
                actionName: "plugins.invoke_ui"
                payload: "{\"plugin\":\"demo_plugin\"}"
                onRequest: (json) => processService.submitRequest(json)
            }

            ActionButton {
                label: "Geometry 包围盒"
                actionName: "geometry.bounding_box"
                payload: "{\"pointCount\":1000000}"
                onRequest: (json) => processService.submitRequest(json)
            }
        }

        BusyIndicator {
            running: processService.busy
            Layout.alignment: Qt.AlignHCenter
            visible: running
        }

        ResponsePanel {
            id: responsePanel
        }
    }

    Connections {
        target: processService

        function onResponseReady(requestId, responseJson) {
            responsePanel.appendResponse(responseJson)
        }

        function onErrorOccurred(requestId, errorMessage) {
            responsePanel.appendResponse("ERROR: " + errorMessage)
        }
    }
}
