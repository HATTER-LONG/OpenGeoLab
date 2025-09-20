import QtQuick
import QtQuick.Controls

import "widgets"

FramelessWindow {
    property FramelessWindow childWindow: FramelessWindow {
        showWhenReady: false
    }

    Button {
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 20
        }
        text: qsTr("Open Child Window")
        onClicked: childWindow.visible = true
    }
}
