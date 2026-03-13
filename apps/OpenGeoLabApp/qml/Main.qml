import QtQuick
import QtQuick.Window

Window {
    id: root

    property string defaultRequestJson: "{\n  \"operation\": \"pickPlaceholderEntity\",\n  \"modelName\": \"Bracket_A01\",\n  \"bodyCount\": 3,\n  \"viewportWidth\": 1280,\n  \"viewportHeight\": 720,\n  \"screenX\": 412,\n  \"screenY\": 248,\n  \"source\": \"qml-ui\",\n  \"requestedBy\": \"Main.qml\"\n}"

    width: 1320
    height: 780
    visible: true
    title: "OpenGeoLab"
    color: "#d9dfd2"

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: "#e9eadf"
            }
            GradientStop {
                position: 0.52
                color: "#cfdac8"
            }
            GradientStop {
                position: 1.0
                color: "#94b0a5"
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 26
        radius: 28
        color: "#f7f3ea"
        border.width: 1
        border.color: "#d6ccbf"
    }

    Rectangle {
        width: parent.width * 0.32
        height: parent.height - 88
        anchors.left: parent.left
        anchors.leftMargin: 42
        anchors.verticalCenter: parent.verticalCenter
        radius: 26
        color: "#20322f"

        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: 25
            color: "transparent"
            border.width: 1
            border.color: "#35554e"
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 28
            anchors.top: parent.top
            anchors.topMargin: 28
            text: "OpenGeoLab"
            color: "#f5f0e8"
            font.pixelSize: 34
            font.bold: true
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 28
            anchors.top: parent.top
            anchors.topMargin: 82
            width: parent.width - 56
            wrapMode: Text.WordWrap
            text: "The app layer now accepts generic module + JSON requests from QML. The default demo uses the selection service, which internally builds placeholder geometry, scene, and render data before resolving a pick result."
            color: "#c0d2c7"
            font.pixelSize: 17
            lineHeight: 1.35
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 28
            anchors.top: parent.top
            anchors.topMargin: 150
            text: "Module"
            color: "#9fc4b6"
            font.pixelSize: 15
            font.bold: true
        }

        Rectangle {
            width: parent.width - 56
            height: 46
            radius: 12
            anchors.left: parent.left
            anchors.leftMargin: 28
            anchors.top: parent.top
            anchors.topMargin: 176
            color: "#2a3f3b"
            border.width: 1
            border.color: "#486964"

            TextInput {
                id: moduleInput

                anchors.fill: parent
                anchors.margins: 12
                text: "selection"
                color: "#f2ebe0"
                font.pixelSize: 18
                selectionColor: "#9fc4b6"
                selectedTextColor: "#19302b"
            }
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 28
            anchors.top: parent.top
            anchors.topMargin: 236
            text: "Request JSON"
            color: "#9fc4b6"
            font.pixelSize: 15
            font.bold: true
        }

        Rectangle {
            width: parent.width - 56
            height: 182
            radius: 18
            anchors.left: parent.left
            anchors.leftMargin: 28
            anchors.top: parent.top
            anchors.topMargin: 262
            color: "#2a3f3b"
            border.width: 1
            border.color: "#486964"

            Flickable {
                anchors.fill: parent
                anchors.margins: 12
                contentWidth: width
                contentHeight: requestEditor.paintedHeight
                clip: true

                TextEdit {
                    id: requestEditor

                    width: parent.width
                    text: root.defaultRequestJson
                    color: "#f2ebe0"
                    font.pixelSize: 15
                    font.family: "Consolas"
                    wrapMode: TextEdit.WrapAnywhere
                    selectByMouse: true
                    selectionColor: "#9fc4b6"
                    selectedTextColor: "#19302b"
                }
            }
        }

        Rectangle {
            id: runButton

            width: parent.width - 56
            height: 64
            radius: 18
            anchors.left: parent.left
            anchors.leftMargin: 28
            anchors.top: parent.top
            anchors.topMargin: 466
            color: buttonArea.pressed ? "#8fb6a6" : "#9fc4b6"

            Text {
                anchors.centerIn: parent
                text: "Dispatch Service Request"
                color: "#19302b"
                font.pixelSize: 20
                font.bold: true
            }

            MouseArea {
                id: buttonArea
                anchors.fill: parent
                onClicked: openGeoLabController.runServiceRequest(moduleInput.text, requestEditor.text)
            }
        }

        Rectangle {
            width: parent.width - 56
            height: 116
            radius: 20
            anchors.left: parent.left
            anchors.leftMargin: 28
            anchors.top: runButton.bottom
            anchors.topMargin: 20
            color: "#2a3f3b"

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.top: parent.top
                anchors.topMargin: 18
                text: openGeoLabController.lastStatus + "  |  module: " + openGeoLabController.lastModule
                color: "#9fc4b6"
                font.pixelSize: 16
                font.bold: true
            }

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.right: parent.right
                anchors.rightMargin: 20
                anchors.top: parent.top
                anchors.topMargin: 48
                wrapMode: Text.WordWrap
                text: openGeoLabController.lastSummary
                color: "#f2ebe0"
                font.pixelSize: 15
                lineHeight: 1.35
            }
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 28
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 30
            width: parent.width - 56
            wrapMode: Text.WordWrap
            text: "The current demo targets the selection module so the placeholder 3D interaction flow can exercise geometry, scene, render, and selection responsibilities through one request."
            color: "#aac1b6"
            font.pixelSize: 15
            lineHeight: 1.35
        }
    }

    Rectangle {
        width: parent.width * 0.58
        height: parent.height - 88
        anchors.right: parent.right
        anchors.rightMargin: 42
        anchors.verticalCenter: parent.verticalCenter
        radius: 26
        color: "#fffaf2"
        border.width: 1
        border.color: "#d4c7b5"

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 30
            anchors.top: parent.top
            anchors.topMargin: 28
            text: "Placeholder 3D Interaction Data Flow"
            color: "#233530"
            font.pixelSize: 30
            font.bold: true
        }

        Rectangle {
            width: parent.width - 60
            height: 224
            anchors.left: parent.left
            anchors.leftMargin: 30
            anchors.top: parent.top
            anchors.topMargin: 88
            radius: 22
            color: "#eef1e6"
            border.width: 1
            border.color: "#ced3c6"

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.top: parent.top
                anchors.topMargin: 18
                text: "Component Response JSON"
                color: "#52655d"
                font.pixelSize: 17
                font.bold: true
            }

            Flickable {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.margins: 18
                anchors.topMargin: 52
                contentWidth: width
                contentHeight: jsonText.paintedHeight
                clip: true

                Text {
                    id: jsonText

                    width: parent.width
                    text: openGeoLabController.lastPayload
                    color: "#233530"
                    font.pixelSize: 15
                    font.family: "Consolas"
                    wrapMode: Text.WrapAnywhere
                }
            }
        }

        Rectangle {
            width: parent.width - 60
            height: 280
            anchors.left: parent.left
            anchors.leftMargin: 30
            anchors.top: parent.top
            anchors.topMargin: 336
            radius: 22
            color: "#f4eee1"
            border.width: 1
            border.color: "#d9ccba"

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.top: parent.top
                anchors.topMargin: 18
                text: "Equivalent Python Automation"
                color: "#695c4a"
                font.pixelSize: 17
                font.bold: true
            }

            Flickable {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.margins: 18
                anchors.topMargin: 52
                contentWidth: width
                contentHeight: scriptText.paintedHeight
                clip: true

                Text {
                    id: scriptText

                    width: parent.width
                    text: openGeoLabController.suggestedPython
                    color: "#40372d"
                    font.pixelSize: 15
                    font.family: "Consolas"
                    wrapMode: Text.WrapAnywhere
                }
            }
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 30
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 28
            width: parent.width - 60
            wrapMode: Text.WordWrap
            text: "The current placeholder stack still lacks a real OpenGL viewport host and command recording, but the service graph now proves the intended geometry to scene to render to selection handoff."
            color: "#5d655f"
            font.pixelSize: 15
            lineHeight: 1.35
        }
    }
}
