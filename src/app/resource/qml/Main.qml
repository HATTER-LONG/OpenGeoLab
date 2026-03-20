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
                label: "Geometry 包围盒 (随机)"
                actionName: "geometry.bounding_box"
                payload: "{\"pointCount\":1000000}"
                onRequest: (json) => processService.submitRequest(json)
            }

            ActionButton {
                label: "Set Points → C++"
                actionName: "geometry.set_points"
                payload: "{\"points\":[{\"x\":1,\"y\":2,\"z\":3},{\"x\":-10,\"y\":20,\"z\":0},{\"x\":100,\"y\":-50,\"z\":25}]}"
                onRequest: (json) => processService.submitRequest(json)
            }

            ActionButton {
                label: "Get Stored BBox ← C++"
                actionName: "geometry.get_stored_bbox"
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
