pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL

Item {
    id: root

    property bool busy: OGL.BackendService.busy
    property bool shown: false
    readonly property string messageText: OGL.BackendService.message.length > 0 ? OGL.BackendService.message : qsTr("Working...")
    readonly property bool messageLikelyLong: messageText.length > 80

    width: 320
    height: visible ? content.implicitHeight + 20 : 0

    visible: shown || hideSequence.running
    opacity: 0.0

    SequentialAnimation {
        id: hideSequence
        running: false

        PauseAnimation {
            duration: 4000
        }
        NumberAnimation {
            target: root
            property: "opacity"
            to: 0.0
            duration: 250
            easing.type: Easing.InOutQuad
        }
        ScriptAction {
            script: {
                if (!root.busy) {
                    root.shown = false;
                }
            }
        }
    }

    Connections {
        target: root
        function onBusyChanged() {
            if (root.busy) {
                hideSequence.stop();
                root.shown = true;
                root.opacity = 1.0;
            } else {
                if (root.shown) {
                    hideSequence.restart();
                }
            }
        }
    }

    Rectangle {
        id: background
        anchors.fill: parent
        radius: 8
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
    }

    ColumnLayout {
        id: content
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                id: messageLabel
                Layout.fillWidth: true
                text: root.messageText
                color: Theme.palette.text
                elide: Text.ElideRight
                font.weight: Font.Medium
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                maximumLineCount: 2
            }

            Button {
                visible: root.messageLikelyLong
                implicitHeight: 24
                flat: true
                text: qsTr("Details")
                onClicked: messagePopup.open()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Show full message")
            }

            // Cancel button
            Button {
                implicitWidth: 24
                implicitHeight: 24
                flat: true
                text: "✕"
                onClicked: OGL.BackendService.cancel()
                Layout.alignment: Qt.AlignTop

                ToolTip.visible: hovered
                ToolTip.text: qsTr("Cancel operation")
            }
        }
        RowLayout {

            Layout.fillWidth: true
            spacing: 8
            ProgressBar {
                id: bar
                Layout.fillWidth: true
                from: 0
                to: 1
                value: OGL.BackendService.progress >= 0 ? OGL.BackendService.progress : 0
                indeterminate: OGL.BackendService.progress < 0
            }

            Label {
                Layout.fillWidth: true
                text: OGL.BackendService.progress >= 0 ? qsTr("%1%").arg(Math.round(OGL.BackendService.progress * 100)) : qsTr("Processing...")
                color: Theme.palette.placeholderText
                horizontalAlignment: Text.AlignRight
                font.pixelSize: 11
            }
        }
    }

    Popup {
        id: messagePopup
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        width: Math.min(520, Math.max(360, root.width))
        parent: Overlay.overlay
        anchors.centerIn: parent

        background: Rectangle {
            radius: 10
            color: Theme.surface
            border.width: 1
            border.color: Theme.border
        }

        contentItem: Item {
            implicitWidth: messagePopup.width
            implicitHeight: layout.implicitHeight + 24

            ColumnLayout {
                id: layout
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Message")
                        font.weight: Font.DemiBold
                        color: Theme.palette.text
                    }

                    Button {
                        text: qsTr("Close")
                        onClicked: messagePopup.close()
                    }
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 220
                    clip: true

                    TextArea {
                        text: root.messageText
                        readOnly: true
                        wrapMode: TextArea.Wrap
                        selectByMouse: true
                        background: Rectangle {
                            radius: 8
                            color: Theme.surface
                            border.width: 1
                            border.color: Theme.border
                        }
                    }
                }
            }
        }
    }
}
