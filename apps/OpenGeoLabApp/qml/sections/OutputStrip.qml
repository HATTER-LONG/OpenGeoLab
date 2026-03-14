pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import "../theme"
import "../components" as Components

Rectangle {
    id: outputStrip

    required property AppTheme theme
    property string responseJson: ""
    property string replayPython: ""
    property string pythonOutput: ""

    radius: theme.radiusLarge
    color: theme.surface
    border.width: 1
    border.color: theme.borderSubtle

    RowLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 12

        Components.CodePanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            theme: outputStrip.theme
            title: "Component Response JSON"
            bodyText: outputStrip.responseJson
        }

        Components.CodePanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            theme: outputStrip.theme
            title: "Recorded Python Replay"
            bodyText: outputStrip.replayPython
        }

        Components.CodePanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            theme: outputStrip.theme
            title: "Python Output"
            bodyText: outputStrip.pythonOutput
        }
    }
}
