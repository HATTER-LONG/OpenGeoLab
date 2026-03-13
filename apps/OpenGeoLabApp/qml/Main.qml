import QtQuick
import QtQuick.Window

Window {
    id: root

    property string defaultRequestJson: "{\n  \"operation\": \"placeholderModel\",\n  \"modelName\": \"Bracket_A01\",\n  \"bodyCount\": 3,\n  \"source\": \"qml-ui\",\n  \"requestedBy\": \"Main.qml\"\n}"

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
            text: "The app layer now accepts generic module + JSON requests from QML. The controller does not hardcode geometry-specific service interfaces anymore."
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
                text: "geometry"
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
            text: "The current demo still targets geometry, but the app/controller contract is now fully generic and only depends on module names plus JSON payloads from QML."
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
            text: "Generic IService Request Evidence"
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
            text: "The placeholder geometry module is intentionally simple, but the request path, target separation, and future extension points now match the modular architecture direction."
            color: "#5d655f"
            font.pixelSize: 15
            lineHeight: 1.35
        }
    }
}
