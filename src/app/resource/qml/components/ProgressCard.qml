pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import OpenGeoLab.Services
import "../theme"

/// @brief Compact progress card aligned to Activity button height.
/// Two rows: icon + message, progress bar + percentage.
Item {
    id: root

    required property AppTheme theme

    readonly property bool active: ProgressTracker.hasActiveTasks
    readonly property real progress: ProgressTracker.currentProgress
    readonly property string description: ProgressTracker.statusText
    readonly property string message: ProgressTracker.currentMessage

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

    visible: root.active || hideTimer.running || root.completionState !== ""
    implicitWidth: 280
    implicitHeight: visible ? cardBg.height : 0
    opacity: visible ? 1 : 0

    Behavior on opacity {
        NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
    }

    Timer {
        id: hideTimer
        repeat: false
        onTriggered: {
            root.completionState = "";
            root.lastDescription = "";
        }
    }

    Connections {
        target: ProgressTracker

        function onTaskStarted(taskId: string) {
            root.freshStart = true;
            root.completionState = "";
            hideTimer.stop();
        }

        function onTaskCompleted(taskId: string, success: bool) {
            root.completionState = success ? "done" : "failed";
            hideTimer.interval = success ? 2000 : 4000;
            hideTimer.restart();
        }
    }

    readonly property string displayText: {
        if (root.completionState === "done")
            return root.lastDescription.length > 0
                ? qsTr("Completed: %1").arg(root.lastDescription) : qsTr("Completed");
        if (root.completionState === "failed")
            return root.lastDescription.length > 0
                ? qsTr("Failed: %1").arg(root.lastDescription) : qsTr("Failed");
        if (root.message.length > 0)
            return root.message;
        return root.description.length > 0 ? root.description : qsTr("Processing…");
    }

    readonly property real displayProgress: {
        if (root.completionState !== "") return 1.0;
        return root.progress;
    }

    readonly property color iconColor: root.completionState === "done"
        ? root.theme.success
        : (root.completionState === "failed" ? root.theme.danger : root.theme.accentA)

    readonly property string statusIcon: root.completionState === "done"
        ? "checkmarkCircleOutline"
        : (root.completionState === "failed" ? "closeCircleOutline" : "hourglassOutline")

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
            anchors.bottomMargin: 5
            spacing: 3

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
                            if (root.freshStart) return 0;
                            if (root.displayProgress <= 0)
                                return Math.max(parent.width * 0.32, 40);
                            return parent.width * Math.max(0, Math.min(1, root.displayProgress));
                        }
                        height: parent.height
                        radius: parent.radius
                        color: root.theme.accentA
                        x: root.displayProgress <= 0 && !root.freshStart ? -width : 0

                        Behavior on width {
                            enabled: root.displayProgress > 0 && root.completionState === ""
                                && !root.freshStart
                            NumberAnimation { duration: 280; easing.type: Easing.OutCubic }
                        }

                        NumberAnimation on x {
                            running: root.displayProgress <= 0 && root.visible
                                && root.completionState === "" && !root.freshStart
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