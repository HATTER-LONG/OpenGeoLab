pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "../theme"

Rectangle {
    id: root

    required property AppTheme theme
    required property var model
    signal commandSubmitted(string text)

    radius: root.theme.radiusMedium
    color: root.theme.surface
    border.width: 1
    border.color: root.theme.borderSubtle
    clip: true

    function submitInput(): void {
        const submittedText = inputEdit.text;
        root.commandSubmitted(submittedText);
        inputEdit.clear();
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: root.theme.tint(root.theme.surface, root.theme.darkMode ? 0.5 : 1.0)

            ListView {
                id: terminalList

                anchors.fill: parent
                anchors.margins: root.theme.shellPadding
                anchors.rightMargin: root.theme.shellPadding + terminalScrollBar.width + root.theme.gapTight
                clip: true
                spacing: root.theme.gapTight
                model: root.model
                boundsBehavior: Flickable.StopAtBounds

                property bool stickToEnd: true

                onCountChanged: {
                    if (stickToEnd) {
                        Qt.callLater(function() {
                            terminalList.positionViewAtEnd();
                        });
                    }
                }

                onMovementEnded: {
                    stickToEnd = atYEnd || contentHeight <= height;
                }

                onFlickEnded: {
                    stickToEnd = atYEnd || contentHeight <= height;
                }

                onContentYChanged: {
                    if (!moving && !flicking) {
                        stickToEnd = atYEnd || contentHeight <= height;
                    }
                }

                delegate: Item {
                    required property string type
                    required property string text

                    readonly property string linePrefix: {
                        if (type === "command") {
                            return ">>> ";
                        }
                        if (type === "response") {
                            return "<<< ";
                        }
                        return "!!! ";
                    }

                    readonly property color lineColor: {
                        if (type === "command") {
                            return root.theme.success;
                        }
                        if (type === "response") {
                            return root.theme.warning;
                        }
                        return root.theme.danger;
                    }

                    width: terminalList.width - terminalList.leftMargin - terminalList.rightMargin
                    implicitHeight: lineText.implicitHeight

                    Text {
                        id: lineText

                        width: parent.width
                        text: parent.linePrefix + parent.text
                        color: parent.lineColor
                        wrapMode: Text.WrapAnywhere
                        font.family: root.theme.monoFontFamily
                        font.pixelSize: 12
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    id: terminalScrollBar

                    width: 6
                    policy: ScrollBar.AsNeeded

                    background: Rectangle {
                        implicitWidth: 6
                        radius: width / 2
                        color: root.theme.tint(root.theme.borderSubtle, 0.3)
                    }

                    contentItem: Rectangle {
                        implicitWidth: 6
                        implicitHeight: Math.max(26, terminalScrollBar.availableHeight * terminalScrollBar.visualSize)
                        radius: width / 2
                        color: root.theme.surfaceStrong
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.theme.borderSubtle
        }

        Rectangle {
            id: inputPanel

            Layout.fillWidth: true
            Layout.preferredHeight: editorContainer.implicitHeight + (root.theme.gapTight * 2)
            color: root.theme.surfaceMuted

            RowLayout {
                id: editorContainer

                anchors.fill: parent
                anchors.margins: root.theme.gapTight
                spacing: root.theme.gapTight

                Text {
                    Layout.alignment: Qt.AlignTop
                    Layout.topMargin: inputEdit.topPadding
                    text: ">"
                    color: root.theme.textTertiary
                    font.family: root.theme.monoFontFamily
                    font.pixelSize: 12
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: inputEditHeight
                    Layout.minimumHeight: inputEditHeight

                    readonly property real inputEditHeight: Math.min(156, Math.max(58, inputEdit.contentHeight + inputEdit.topPadding + inputEdit.bottomPadding))

                    TextEdit {
                        id: inputEdit

                        anchors.fill: parent
                        wrapMode: TextEdit.WrapAnywhere
                        color: root.theme.textPrimary
                        font.family: root.theme.monoFontFamily
                        font.pixelSize: 12
                        selectionColor: root.theme.tint(root.theme.accentA, 0.35)
                        selectedTextColor: root.theme.textPrimary
                        topPadding: 10
                        bottomPadding: 10
                        leftPadding: 0
                        rightPadding: 0

                        Keys.onPressed: function(event) {
                            if ((event.modifiers & Qt.ControlModifier)
                                    && (event.key === Qt.Key_Return || event.key === Qt.Key_Enter)) {
                                root.submitInput();
                                event.accepted = true;
                            }
                        }
                    }

                    Text {
                        anchors.left: inputEdit.left
                        anchors.top: inputEdit.top
                        anchors.leftMargin: inputEdit.leftPadding
                        anchors.topMargin: inputEdit.topPadding
                        text: qsTr("Type a command...")
                        color: root.theme.textTertiary
                        font.family: root.theme.monoFontFamily
                        font.pixelSize: 12
                        visible: inputEdit.text.length === 0
                    }
                }

                Rectangle {
                    Layout.alignment: Qt.AlignTop
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    radius: root.theme.radiusSmall
                    color: runMouseArea.pressed ? root.theme.surfaceStrong : root.theme.surface
                    border.width: 1
                    border.color: runMouseArea.containsMouse ? root.theme.tint(root.theme.accentA, 0.45) : root.theme.borderSubtle

                    AppIcon {
                        anchors.centerIn: parent
                        theme: root.theme
                        iconKind: "play"
                        width: 16
                        height: 16
                    }

                    MouseArea {
                        id: runMouseArea

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.submitInput()
                    }

                    ToolTip {
                        visible: runMouseArea.containsMouse
                        delay: 600
                        text: qsTr("Run")
                    }
                }
            }
        }
    }
}
