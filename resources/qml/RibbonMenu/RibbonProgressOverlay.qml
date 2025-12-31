pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL

/**
 * Non-blocking progress indicator displayed in the bottom-right corner.
 * Shows operation status without capturing input or freezing the UI.
 * Parent should set anchors for positioning.
 */
Item {
    id: root

    width: 320
    height: visible ? content.height + 20 : 0

    visible: OGL.BackendService.busy
    opacity: visible ? 1.0 : 0.0

    Behavior on opacity {
        NumberAnimation {
            duration: 200
        }
    }

    Rectangle {
        id: background
        anchors.fill: parent
        radius: 8
        color: Theme.surfaceColor
        border.width: 1
        border.color: Theme.borderColor
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

            Label {
                Layout.fillWidth: true
                text: OGL.BackendService.message.length > 0 ? OGL.BackendService.message : qsTr("Working...")
                color: Theme.textPrimaryColor
                elide: Text.ElideRight
                font.weight: Font.Medium
            }

            // Cancel button
            Button {
                implicitWidth: 24
                implicitHeight: 24
                flat: true
                text: "✕"
                onClicked: OGL.BackendService.cancel()

                ToolTip.visible: hovered
                ToolTip.text: qsTr("Cancel operation")
            }
        }

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
            color: Theme.textSecondaryColor
            horizontalAlignment: Text.AlignRight
            font.pixelSize: 11
        }
    }
}
