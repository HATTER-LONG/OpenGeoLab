import QtQuick
import QtQuick.Layouts
import QtQuick.Window

Window {
    id: root

    required property var appController
    property string defaultRequestJson: "{\n  \"operation\": \"pickPlaceholderEntity\",\n  \"modelName\": \"Bracket_A01\",\n  \"bodyCount\": 3,\n  \"viewportWidth\": 1280,\n  \"viewportHeight\": 720,\n  \"screenX\": 412,\n  \"screenY\": 248,\n  \"source\": \"qml-ui\",\n  \"requestedBy\": \"Main.qml\"\n}"
    property string defaultPythonScript: "import opengeolab_app\n\nselection = opengeolab_app.run_command(\n    \"selection\",\n    {\n        \"operation\": \"pickPlaceholderEntity\",\n        \"modelName\": \"Bracket_A01\",\n        \"bodyCount\": 2,\n        \"viewportWidth\": 960,\n        \"viewportHeight\": 540,\n        \"screenX\": 180,\n        \"screenY\": 120\n    },\n)\nprint(selection[\"payload\"][\"summary\"])\nprint(opengeolab_app.get_state()[\"recordedCommandCount\"])"
    property string defaultPythonCommandLine: "opengeolab_app.get_state()['recordedCommandCount']"

    width: 1320
    height: 820
    minimumWidth: 1100
    minimumHeight: 760
    visible: true
    title: "OpenGeoLab"
    color: "#d9dfd2"

    component ActionButton: Rectangle {
        id: actionButton

        property string buttonText: ""
        property color buttonColor: "#d9e1f3"
        property color pressedColor: "#cad5ec"
        property color labelColor: "#2a3659"
        property int labelSize: 14
        signal clicked

        width: 120
        height: 40
        radius: 12
        color: buttonArea.pressed ? pressedColor : buttonColor

        Text {
            anchors.centerIn: parent
            text: actionButton.buttonText
            color: actionButton.labelColor
            font.pixelSize: actionButton.labelSize
            font.bold: true
        }

        MouseArea {
            id: buttonArea
            anchors.fill: parent
            onClicked: actionButton.clicked()
        }
    }

    component CodeSurface: Rectangle {
        id: codeSurface

        property alias title: titleText.text
        property alias bodyText: bodyText.text
        property color panelColor: "#f8faff"
        property color borderColor: "#d7dfef"
        property color textColor: "#2a3659"
        property color titleColor: "#61708e"
        property int preferredHeight: 160

        radius: 18
        color: panelColor
        border.width: 1
        border.color: borderColor
        height: preferredHeight

        Text {
            id: titleText

            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.top: parent.top
            anchors.topMargin: 14
            color: codeSurface.titleColor
            font.pixelSize: 14
            font.bold: true
        }

        Flickable {
            anchors.fill: parent
            anchors.margins: 14
            anchors.topMargin: 42
            contentWidth: width
            contentHeight: bodyText.paintedHeight
            clip: true

            Text {
                id: bodyText

                width: parent.width
                color: codeSurface.textColor
                font.pixelSize: 13
                font.family: "Consolas"
                wrapMode: Text.WrapAnywhere
            }
        }
    }

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
        id: frame

        anchors.fill: parent
        anchors.margins: 26
        radius: 28
        color: "#f7f3ea"
        border.width: 1
        border.color: "#d6ccbf"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 18

            Rectangle {
                Layout.preferredWidth: Math.max(360, frame.width * 0.31)
                Layout.fillHeight: true
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

                Flickable {
                    anchors.fill: parent
                    anchors.margins: 24
                    contentWidth: width
                    contentHeight: leftColumn.implicitHeight
                    clip: true

                    Column {
                        id: leftColumn

                        width: parent.width
                        spacing: 16

                        Text {
                            text: "OpenGeoLab"
                            color: "#f5f0e8"
                            font.pixelSize: 34
                            font.bold: true
                        }

                        Text {
                            width: parent.width
                            wrapMode: Text.WordWrap
                            text: "The app layer now accepts generic module + JSON requests from QML and also hosts an in-process Python API. Both entry points converge on the same command pipeline and recorded application state."
                            color: "#c0d2c7"
                            font.pixelSize: 17
                            lineHeight: 1.35
                        }

                        Text {
                            text: "Module"
                            color: "#9fc4b6"
                            font.pixelSize: 15
                            font.bold: true
                        }

                        Rectangle {
                            width: parent.width
                            height: 46
                            radius: 12
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
                            text: "Request JSON"
                            color: "#9fc4b6"
                            font.pixelSize: 15
                            font.bold: true
                        }

                        Rectangle {
                            width: parent.width
                            height: 208
                            radius: 18
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

                        ActionButton {
                            width: parent.width
                            height: 62
                            radius: 18
                            buttonText: "Dispatch Service Request"
                            buttonColor: "#9fc4b6"
                            pressedColor: "#8fb6a6"
                            labelColor: "#19302b"
                            labelSize: 20
                            onClicked: root.appController.runServiceRequest(moduleInput.text, requestEditor.text)
                        }

                        Flow {
                            width: parent.width
                            spacing: 12

                            ActionButton {
                                width: 150
                                buttonText: "Replay History"
                                buttonColor: "#eadfcb"
                                pressedColor: "#d8d0bd"
                                labelColor: "#40372d"
                                labelSize: 16
                                onClicked: root.appController.replayRecordedCommands()
                            }

                            ActionButton {
                                width: 126
                                buttonText: "Clear History"
                                buttonColor: "#e3d6c8"
                                pressedColor: "#d0c5b8"
                                labelColor: "#40372d"
                                labelSize: 16
                                onClicked: root.appController.clearRecordedCommands()
                            }
                        }

                        Rectangle {
                            width: parent.width
                            radius: 20
                            color: "#2a3f3b"
                            border.width: 1
                            border.color: "#35554e"
                            implicitHeight: 126

                            Column {
                                anchors.fill: parent
                                anchors.margins: 18
                                spacing: 10

                                Text {
                                    width: parent.width
                                    text: root.appController.lastStatus + "  |  module: " + root.appController.lastModule
                                    color: "#9fc4b6"
                                    font.pixelSize: 16
                                    font.bold: true
                                    wrapMode: Text.WrapAnywhere
                                }

                                Text {
                                    width: parent.width
                                    wrapMode: Text.WordWrap
                                    text: root.appController.lastSummary
                                    color: "#f2ebe0"
                                    font.pixelSize: 15
                                    lineHeight: 1.35
                                }
                            }
                        }

                        Text {
                            width: parent.width
                            wrapMode: Text.WordWrap
                            text: "Recorded commands: " + root.appController.recordedCommandCount + ". The UI executes through the command layer, which can replay the captured history and export a Python replay script."
                            color: "#aac1b6"
                            font.pixelSize: 15
                            lineHeight: 1.35
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 26
                color: "#fffaf2"
                border.width: 1
                border.color: "#d4c7b5"

                Flickable {
                    anchors.fill: parent
                    anchors.margins: 24
                    contentWidth: width
                    contentHeight: rightColumn.implicitHeight
                    clip: true

                    Column {
                        id: rightColumn

                        width: parent.width
                        spacing: 18

                        Text {
                            width: parent.width
                            text: "Placeholder 3D Interaction Data Flow"
                            color: "#233530"
                            font.pixelSize: 30
                            font.bold: true
                            wrapMode: Text.WordWrap
                        }

                        CodeSurface {
                            width: parent.width
                            preferredHeight: 178
                            panelColor: "#eef1e6"
                            borderColor: "#ced3c6"
                            titleColor: "#52655d"
                            textColor: "#233530"
                            title: "Component Response JSON"
                            bodyText: root.appController.lastPayload
                        }

                        CodeSurface {
                            width: parent.width
                            preferredHeight: 146
                            panelColor: "#f4eee1"
                            borderColor: "#d9ccba"
                            titleColor: "#695c4a"
                            textColor: "#40372d"
                            title: "Recorded Python Replay Script"
                            bodyText: root.appController.suggestedPython
                        }

                        Rectangle {
                            width: parent.width
                            radius: 22
                            color: "#edf0f7"
                            border.width: 1
                            border.color: "#cad2e4"
                            implicitHeight: embeddedColumn.implicitHeight + 36

                            Column {
                                id: embeddedColumn

                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 18
                                spacing: 14

                                Text {
                                    text: "Embedded Python Runtime"
                                    color: "#3f4d72"
                                    font.pixelSize: 17
                                    font.bold: true
                                }

                                Flow {
                                    width: parent.width
                                    spacing: 10

                                    ActionButton {
                                        width: 108
                                        buttonText: "Run Script"
                                        buttonColor: "#c8d1e8"
                                        pressedColor: "#b9c3dd"
                                        labelColor: "#2a3659"
                                        labelSize: 15
                                        onClicked: root.appController.runEmbeddedPython(pythonEditor.text)
                                    }

                                    ActionButton {
                                        width: 96
                                        buttonText: "Record Test"
                                        buttonColor: "#e2e8f5"
                                        pressedColor: "#d6dfef"
                                        labelColor: "#2a3659"
                                        labelSize: 14
                                        onClicked: root.appController.recordSelectionSmokeTest()
                                    }

                                    ActionButton {
                                        width: 96
                                        buttonText: "Replay Test"
                                        buttonColor: "#e2e8f5"
                                        pressedColor: "#d6dfef"
                                        labelColor: "#2a3659"
                                        labelSize: 14
                                        onClicked: root.appController.replayRecordedCommands()
                                    }
                                }

                                Rectangle {
                                    width: parent.width
                                    height: 44
                                    radius: 14
                                    color: "#f8faff"
                                    border.width: 1
                                    border.color: "#d7dfef"

                                    TextInput {
                                        id: pythonCommandLineInput

                                        anchors.left: parent.left
                                        anchors.leftMargin: 12
                                        anchors.right: executeLineButton.left
                                        anchors.rightMargin: 12
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: root.defaultPythonCommandLine
                                        color: "#2a3659"
                                        font.pixelSize: 14
                                        font.family: "Consolas"
                                        selectionColor: "#b9c3dd"
                                        selectedTextColor: "#24324f"
                                        clip: true
                                        onAccepted: root.appController.runEmbeddedPythonCommandLine(text)
                                    }

                                    ActionButton {
                                        id: executeLineButton

                                        anchors.right: parent.right
                                        anchors.rightMargin: 7
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 104
                                        height: 30
                                        radius: 10
                                        buttonText: "Execute Line"
                                        buttonColor: "#d9e1f3"
                                        pressedColor: "#cad5ec"
                                        labelColor: "#2a3659"
                                        labelSize: 13
                                        onClicked: root.appController.runEmbeddedPythonCommandLine(pythonCommandLineInput.text)
                                    }
                                }

                                RowLayout {
                                    width: parent.width
                                    spacing: 12

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredWidth: parent.width * 0.58
                                        height: 150
                                        radius: 16
                                        color: "#f8faff"
                                        border.width: 1
                                        border.color: "#d7dfef"

                                        Flickable {
                                            anchors.fill: parent
                                            anchors.margins: 12
                                            contentWidth: width
                                            contentHeight: pythonEditor.paintedHeight
                                            clip: true

                                            TextEdit {
                                                id: pythonEditor

                                                width: parent.width
                                                text: root.defaultPythonScript
                                                color: "#2a3659"
                                                font.pixelSize: 14
                                                font.family: "Consolas"
                                                wrapMode: TextEdit.WrapAnywhere
                                                selectByMouse: true
                                            }
                                        }
                                    }

                                    CodeSurface {
                                        Layout.fillWidth: true
                                        Layout.preferredWidth: parent.width * 0.34
                                        preferredHeight: 150
                                        panelColor: "#f8faff"
                                        borderColor: "#d7dfef"
                                        titleColor: "#61708e"
                                        textColor: "#2a3659"
                                        title: "stdout / stderr"
                                        bodyText: root.appController.lastPythonOutput
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
