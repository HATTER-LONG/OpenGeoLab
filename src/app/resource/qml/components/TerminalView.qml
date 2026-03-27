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

    Popup {
        id: terminalContextMenu

        property string targetJson: ""

        width: copyRow.implicitWidth + 24
        height: copyRow.implicitHeight + 16
        padding: 0
        modal: false
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            radius: root.theme.radiusMedium
            color: root.theme.surfaceStrong
            border.width: 1
            border.color: root.theme.tint(root.theme.borderSubtle, root.theme.darkMode ? 0.8 : 0.5)
        }

        contentItem: Rectangle {
            color: copyMouseArea.containsMouse ? root.theme.tint(root.theme.accentA, root.theme.darkMode ? 0.18 : 0.08) : root.theme.surfaceStrong
            radius: root.theme.radiusSmall

            Row {
                id: copyRow
                anchors.centerIn: parent
                spacing: 6

                AppIcon {
                    theme: root.theme
                    iconKind: "copyOutline"
                    useThemeContrast: false
                    primaryColor: root.theme.textPrimary
                    width: 14
                    height: 14
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: qsTr("Copy")
                    color: root.theme.textPrimary
                    font.pixelSize: 12
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            MouseArea {
                id: copyMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.copyToClipboard(terminalContextMenu.targetJson);
                    terminalContextMenu.close();
                }
            }
        }
    }

    function copyToClipboard(text: string): void {
        clipboardHelper.text = text;
        clipboardHelper.selectAll();
        clipboardHelper.copy();
    }

    TextEdit {
        id: clipboardHelper
        visible: false
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
                anchors.rightMargin: root.theme.gapTight
                rightMargin: terminalScrollBar.width + root.theme.gapTight
                clip: true
                spacing: root.theme.gapTight
                model: root.model
                boundsBehavior: Flickable.StopAtBounds

                property bool stickToEnd: true

                onCountChanged: {
                    if (stickToEnd) {
                        Qt.callLater(function () {
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
                    required property string header
                    required property string json

                    readonly property string linePrefix: {
                        if (type === "command") {
                            return ">>> ";
                        }
                        if (type === "response") {
                            return "<<< ";
                        }
                        return "!!! ";
                    }

                    readonly property string displayText: {
                        if (header.length > 0)
                            return linePrefix + header + "\n" + json;
                        return linePrefix + json;
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
                    implicitHeight: lineEdit.implicitHeight

                    TextEdit {
                        id: lineEdit

                        width: parent.width
                        text: parent.displayText
                        color: parent.lineColor
                        wrapMode: TextEdit.WrapAnywhere
                        font.family: root.theme.monoFontFamily
                        font.pixelSize: 12
                        readOnly: true
                        selectByMouse: true
                        selectionColor: root.theme.tint(root.theme.accentA, 0.35)
                        selectedTextColor: root.theme.textPrimary

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.RightButton
                            cursorShape: Qt.IBeamCursor
                            onClicked: function (mouse) {
                                terminalContextMenu.targetJson = parent.parent.json;
                                const pos = mapToItem(root, mouse.x, mouse.y);
                                terminalContextMenu.x = pos.x;
                                terminalContextMenu.y = pos.y;
                                terminalContextMenu.open();
                            }
                        }
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
                        implicitWidth: terminalScrollBar.hovered ? 8 : 6
                        implicitHeight: Math.max(26, terminalScrollBar.availableHeight * terminalScrollBar.visualSize)
                        radius: width / 2
                        color: terminalScrollBar.hovered ? root.theme.tint(root.theme.textSecondary, root.theme.darkMode ? 0.6 : 0.4) : root.theme.surfaceStrong

                        Behavior on implicitWidth {
                            NumberAnimation {
                                duration: 120
                            }
                        }
                        Behavior on color {
                            ColorAnimation {
                                duration: 120
                            }
                        }
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

                Flickable {
                    id: inputFlickable

                    Layout.fillWidth: true
                    Layout.preferredHeight: inputEditHeight
                    Layout.minimumHeight: inputEditHeight

                    readonly property real inputEditHeight: Math.min(156, Math.max(58, inputEdit.contentHeight + inputEdit.topPadding + inputEdit.bottomPadding))

                    contentWidth: width
                    contentHeight: inputEdit.contentHeight + inputEdit.topPadding + inputEdit.bottomPadding
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    flickableDirection: Flickable.VerticalFlick

                    function ensureCursorVisible() {
                        const cursorY = inputEdit.cursorRectangle.y;
                        const cursorH = inputEdit.cursorRectangle.height;
                        if (cursorY < contentY)
                            contentY = cursorY;
                        else if (cursorY + cursorH > contentY + height)
                            contentY = cursorY + cursorH - height;
                    }

                    TextEdit {
                        id: inputEdit

                        width: inputFlickable.width
                        height: Math.max(inputFlickable.height, contentHeight + topPadding + bottomPadding)
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

                        onCursorRectangleChanged: inputFlickable.ensureCursorVisible()

                        Keys.onPressed: function (event) {
                            if ((event.modifiers & Qt.ControlModifier) && (event.key === Qt.Key_Return || event.key === Qt.Key_Enter)) {
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

                    ScrollBar.vertical: ScrollBar {
                        width: 4
                        policy: ScrollBar.AsNeeded

                        contentItem: Rectangle {
                            implicitWidth: 4
                            radius: 2
                            color: root.theme.surfaceStrong
                        }
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
