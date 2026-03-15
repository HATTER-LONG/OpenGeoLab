pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import "../theme"

Item {
    id: root

    required property AppTheme theme
    required property var appController
    property bool shown: false
    readonly property bool active: appController && appController.operationActive
    readonly property real progressValue: appController ? appController.operationProgress : -1
    readonly property string state: appController ? appController.operationState : "idle"
    readonly property string messageText: appController && appController.operationMessage.length > 0
        ? appController.operationMessage
        : qsTr("Working...")
    readonly property bool indeterminate: active && progressValue < 0
    readonly property bool terminalState: !active && (state === "success" || state === "error")
    readonly property color accentColor: {
        if (state === "error") {
            return theme.accentD;
        }
        if (state === "success") {
            return theme.accentB;
        }
        return theme.accentA;
    }
    readonly property string statusText: {
        if (active && progressValue >= 0) {
            return qsTr("%1%").arg(Math.round(Math.max(0, Math.min(1, progressValue)) * 100));
        }
        if (active) {
            return qsTr("Running");
        }
        if (state === "success") {
            return qsTr("Done");
        }
        if (state === "error") {
            return qsTr("Failed");
        }
        return qsTr("Idle");
    }

    width: 380
    height: visible ? contentColumn.implicitHeight + 20 : 0
    visible: shown || opacity > 0.01
    opacity: shown ? 1 : 0
    y: shown ? 0 : 8

    function syncVisibility() {
        if (!appController) {
            shown = false;
            hideTimer.stop();
            return;
        }

        if (active) {
            hideTimer.stop();
            shown = true;
            return;
        }

        if (terminalState) {
            shown = true;
            hideTimer.interval = state === "error" ? 6200 : 3200;
            hideTimer.restart();
            return;
        }

        hideTimer.stop();
        shown = false;
    }

    function closeImmediately() {
        hideTimer.stop();
        shown = false;
    }

    Behavior on opacity {
        NumberAnimation {
            duration: 180
            easing.type: Easing.OutQuad
        }
    }

    Behavior on y {
        NumberAnimation {
            duration: 180
            easing.type: Easing.OutQuad
        }
    }

    Timer {
        id: hideTimer

        interval: 3200
        repeat: false
        onTriggered: root.shown = false
    }

    Connections {
        target: root.appController

        function onOperationActiveChanged() {
            root.syncVisibility();
        }

        function onOperationStateChanged() {
            root.syncVisibility();
        }

        function onOperationMessageChanged() {
            if (root.appController && root.appController.operationState !== "idle") {
                root.syncVisibility();
            }
        }
    }

    Component.onCompleted: syncVisibility()

    Rectangle {
        anchors.fill: parent
        radius: root.theme.radiusMedium
        color: root.theme.tint(root.theme.surface, root.theme.darkMode ? 0.96 : 1.0)
        border.width: 1
        border.color: root.theme.tint(root.accentColor, root.theme.darkMode ? 0.46 : 0.24)
    }

    ColumnLayout {
        id: contentColumn

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                Layout.fillWidth: true
                text: root.messageText
                color: root.theme.textPrimary
                font.pixelSize: 12
                font.bold: true
                font.family: root.theme.bodyFontFamily
                wrapMode: Text.WordWrap
                maximumLineCount: 2
            }

            Rectangle {
                implicitWidth: 26
                implicitHeight: 26
                radius: root.theme.radiusSmall
                color: closeMouseArea.pressed ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.88 : 0.98)
                                             : (closeMouseArea.containsMouse ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.78 : 0.94) : "transparent")
                border.width: closeMouseArea.containsMouse ? 1 : 0
                border.color: root.theme.tint(root.theme.borderSubtle, 0.82)

                Text {
                    anchors.centerIn: parent
                    text: "\u00d7"
                    color: root.theme.textPrimary
                    font.pixelSize: 13
                    font.bold: true
                    font.family: root.theme.bodyFontFamily
                }

                MouseArea {
                    id: closeMouseArea

                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.closeImmediately()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                id: progressTrack

                Layout.fillWidth: true
                implicitHeight: 10
                radius: 5
                color: root.theme.tint(root.theme.surfaceMuted, root.theme.darkMode ? 0.84 : 0.96)
                border.width: 1
                border.color: root.theme.tint(root.theme.borderSubtle, root.theme.darkMode ? 0.72 : 0.4)
                clip: true

                Rectangle {
                    visible: !root.indeterminate && (root.active || root.terminalState)
                    width: root.terminalState ? parent.width : Math.max(20, parent.width * Math.max(0.08, Math.min(1, root.progressValue)))
                    height: parent.height
                    radius: parent.radius
                    color: root.accentColor
                }

                Rectangle {
                    id: indeterminateBlock

                    visible: root.indeterminate
                    width: Math.max(46, progressTrack.width * 0.3)
                    height: progressTrack.height
                    radius: progressTrack.radius
                    color: root.accentColor
                    x: -width

                    NumberAnimation on x {
                        running: indeterminateBlock.visible
                        from: -indeterminateBlock.width
                        to: progressTrack.width
                        duration: 1100
                        loops: Animation.Infinite
                        easing.type: Easing.InOutQuad
                    }
                }
            }

            Text {
                text: root.statusText
                color: root.theme.textSecondary
                font.pixelSize: 11
                font.bold: true
                font.family: root.theme.bodyFontFamily
            }
        }
    }
}
