import QtQuick.Controls

FramelessWindow {
    id: root
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
        onClicked: root.childWindow.visible = true
    }
}
