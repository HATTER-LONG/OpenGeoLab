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
                label: "启动 QML 插件窗口"
                actionName: "plugins.invoke_ui"
                payload: "{\"plugin\":\"qml_demo_plugin\"}"
                onRequest: (json) => processService.submitRequest(json)
            }

            ActionButton {
                label: "Geometry 包围盒 (随机)"
                moduleName: "geometry"
                actionName: "bounding_box"
                payload: "{\"pointCount\":1000000}"
                onRequest: (json) => processService.submitRequest(json)
            }

            ActionButton {
                label: "Set Points → C++"
                moduleName: "geometry"
                actionName: "set_points"
                payload: "{\"points\":[{\"x\":1,\"y\":2,\"z\":3},{\"x\":-10,\"y\":20,\"z\":0},{\"x\":100,\"y\":-50,\"z\":25}]}"
                onRequest: (json) => processService.submitRequest(json)
            }

            ActionButton {
                label: "Get Stored BBox ← C++"
                moduleName: "geometry"
                actionName: "get_stored_bbox"
                onRequest: (json) => processService.submitRequest(json)
            }

            ActionButton {
                label: "开始录制"
                actionName: "recording.start"
                onRequest: (json) => processService.submitRequest(json)
            }

            ActionButton {
                label: "停止录制"
                actionName: "recording.stop"
                onRequest: (json) => processService.submitRequest(json)
            }

            ActionButton {
                label: "导出脚本"
                actionName: "recording.export"
                payload: "{\"path\": \"session.py\"}"
                onRequest: (json) => processService.submitRequest(json)
            }

            ActionButton {
                label: "回放脚本"
                actionName: "recording.replay"
                payload: "{\"path\": \"session.py\"}"
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
