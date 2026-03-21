import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: mainWindow

    width: 1200
    height: 700
    visible: true
    title: "OpenGeoLab"

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        CoinQuickItem {
            id: viewer3D
            SplitView.preferredWidth: parent.width * 0.6
            SplitView.fillHeight: true

            Component.onCompleted: viewAll()

            onNavigationFinished: (cameraJson) => {
                let req = {
                    module: "render",
                    action: "camera.set_state",
                    requestId: "nav-" + Date.now(),
                    payload: JSON.parse(cameraJson)
                };
                processService.submitRequest(JSON.stringify(req));
            }
        }

        ColumnLayout {
            SplitView.preferredWidth: parent.width * 0.4
            SplitView.fillHeight: true
            spacing: 12

            Label {
                text: "OpenGeoLab"
                font.pixelSize: 20
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 16
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                ColumnLayout {
                    width: parent.width
                    spacing: 8

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

                        ActionButton {
                            label: "获取相机状态"
                            moduleName: "render"
                            actionName: "camera.get_state"
                            onRequest: (json) => processService.submitRequest(json)
                        }

                        ActionButton {
                            label: "添加 Box"
                            moduleName: "render"
                            actionName: "scene.add_box"
                            payload: '{"sizeX":3,"sizeY":2,"sizeZ":1}'
                            onRequest: (json) => processService.submitRequest(json)
                        }

                        ActionButton {
                            label: "描述场景"
                            moduleName: "render"
                            actionName: "scene.describe"
                            onRequest: (json) => processService.submitRequest(json)
                        }

                        ActionButton {
                            label: "View All"
                            moduleName: "render"
                            actionName: "camera.view_all"
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
            }
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
