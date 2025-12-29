pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL

Item {
    id: root

    // Non-blocking progress: shows status without capturing input.
    visible: OGL.BackendService.busy

    anchors.right: parent.right
    anchors.top: parent.top
    anchors.margins: 12

    width: 320
    height: implicitHeight

    Rectangle {
        anchors.fill: content
        radius: 6
        color: Theme.surfaceColor
        border.width: 1
        border.color: Theme.borderColor
        opacity: 0.98
    }

    ColumnLayout {
        id: content
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: OGL.BackendService.message.length > 0 ? OGL.BackendService.message : qsTr("Working...")
            color: Theme.textPrimaryColor
            elide: Text.ElideRight
        }

        ProgressBar {
            id: bar
            Layout.fillWidth: true
            from: 0
            to: 1
            value: OGL.BackendService.progress
            indeterminate: OGL.BackendService.progress < 0
        }

        Label {
            Layout.fillWidth: true
            text: OGL.BackendService.progress >= 0 ? qsTr("%1%").arg(Math.round(OGL.BackendService.progress * 100)) : qsTr("...")
            color: Theme.textSecondaryColor
            horizontalAlignment: Text.AlignRight
        }
    }
}
