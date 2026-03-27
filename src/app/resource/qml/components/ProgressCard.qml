pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import OpenGeoLab.Services 1.0
import "../theme"

/**
 * @brief Compact progress card showing operation status.
 *
 * Two rows: icon + message, progress bar + percentage.
 * Connects directly to RequestService signals.
 */
Item {
    id: root

    required property AppTheme theme

    property bool active: false
    property real progress: 0
    property string description: ""
    property string message: ""

    property string completionState: ""
    property string lastDescription: ""
    property bool freshStart: true

    onDescriptionChanged: {
        if (root.description.length > 0)
            root.lastDescription = root.description;
    }

    onProgressChanged: {
        if (root.progress > 0)
            root.freshStart = false;
    }

    Connections {
        target: RequestService

        function onRequestSent(desc: string, requestJson: string, muted: bool): void {
            if (muted)
                return;
            root.active = true;
            root.progress = 0;
            root.description = desc;
            root.message = "";
            root.freshStart = true;
            root.completionState = "";
        }

        function onResponseReady(responseJson: string, muted: bool): void {
            if (muted)
                return;
            root.completionState = "done";
            root.active = false;
            hideTimer.interval = 3000;
            hideTimer.restart();
        }

        function onErrorOccurred(errorMessage: string, muted: bool): void {
            if (muted)
                return;
            root.completionState = "failed";
            root.active = false;
            hideTimer.interval = 4000;
            hideTimer.restart();
        }

        function onProgressUpdated(prog: double, msg: string): void {
            root.progress = prog;
            root.message = msg;
        }
    }

    readonly property bool shown: root.active || hideTimer.running || root.completionState !== ""

    visible: shown || slideAnimation.running
    implicitWidth: 280
    implicitHeight: shown ? cardBg.height : 0
    clip: true

    Behavior on implicitHeight {
        NumberAnimation {
            id: slideAnimation
            duration: 220
            easing.type: Easing.OutCubic
        }
    }

    opacity: shown ? 1 : 0

    Behavior on opacity {
        NumberAnimation {
            duration: 180
            easing.type: Easing.OutQuad
        }
    }

    Timer {
        id: hideTimer
        repeat: false
        onTriggered: {
            root.completionState = "";
            root.lastDescription = "";
        }
    }

    readonly property string displayText: {
        if (root.completionState === "done")
            return root.lastDescription.length > 0 ? qsTr("Completed: %1").arg(root.lastDescription) : qsTr("Completed");
        if (root.completionState === "failed")
            return root.lastDescription.length > 0 ? qsTr("Failed: %1").arg(root.lastDescription) : qsTr("Failed");
        if (root.message.length > 0)
            return root.message;
        return root.description.length > 0 ? root.description : qsTr("Processing…");
    }

    readonly property real displayProgress: {
        if (root.completionState !== "")
            return 1.0;
        return root.progress;
    }

    readonly property color iconColor: root.completionState === "done" ? root.theme.success : (root.completionState === "failed" ? root.theme.danger : root.theme.accentA)

    readonly property string statusIcon: root.completionState === "done" ? "checkmarkCircleOutline" : (root.completionState === "failed" ? "closeCircleOutline" : "syncOutline")

    Rectangle {
        id: cardBg

        width: root.width
        height: 44
        radius: root.theme.radiusSmall
        color: root.theme.surface
        border.width: 1
        border.color: root.theme.tint(root.theme.borderSubtle, 0.7)

        ColumnLayout {
            id: cardLayout

            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            anchors.topMargin: 5
            anchors.bottomMargin: 3
            spacing: 2

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                AppIcon {
                    theme: root.theme
                    iconKind: root.statusIcon
                    primaryColor: root.iconColor
                    useThemeContrast: false
                    width: 14
                    height: 14
                }

                Text {
                    Layout.fillWidth: true
                    text: root.displayText
                    font.pixelSize: 11
                    font.weight: Font.Medium
                    color: root.theme.textPrimary
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Rectangle {
                    Layout.fillWidth: true
                    height: 4
                    radius: 2
                    color: root.theme.tint(root.theme.borderSubtle, 0.3)
                    clip: true

                    Rectangle {
                        id: progressFill

                        width: {
                            if (root.freshStart)
                                return 0;
                            if (root.displayProgress <= 0)
                                return Math.max(parent.width * 0.32, 40);
                            return parent.width * Math.max(0, Math.min(1, root.displayProgress));
                        }
                        height: parent.height
                        radius: parent.radius
                        color: root.theme.accentA
                        x: root.displayProgress <= 0 && !root.freshStart ? -width : 0

                        Behavior on width {
                            enabled: root.displayProgress > 0 && root.completionState === "" && !root.freshStart
                            NumberAnimation {
                                duration: 280
                                easing.type: Easing.OutCubic
                            }
                        }

                        NumberAnimation on x {
                            running: root.displayProgress <= 0 && root.visible && root.completionState === "" && !root.freshStart
                            from: -progressFill.parent.width * 0.32
                            to: progressFill.parent.width
                            duration: 1200
                            loops: Animation.Infinite
                        }
                    }
                }

                Text {
                    visible: root.displayProgress > 0
                    text: Math.round(root.displayProgress * 100) + "%"
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                    color: root.theme.accentA
                    Layout.minimumWidth: 28
                    horizontalAlignment: Text.AlignRight
                }
            }
        }
    }
}
