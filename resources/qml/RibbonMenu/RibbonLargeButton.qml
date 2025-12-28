pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects

AbstractButton {
    id: root

    property string iconSource: ""
    property string tooltipText: ""

    // Theme-injected colors.
    property color iconColor: Theme.ribbonIconColor
    property color textColor: Theme.ribbonTextColor
    property color hoverColor: Theme.ribbonHoverColor
    property color pressedColor: Theme.ribbonPressedColor

    // Default size (can be overridden by Layout.preferred* in containers).
    implicitWidth: 52
    implicitHeight: 60

    hoverEnabled: true
    focusPolicy: Qt.TabFocus

    background: Rectangle {
        radius: 3
        border.width: root.hovered || root.activeFocus ? 1 : 0
        border.color: root.hoverColor

        color: root.pressed ? root.pressedColor : (root.hovered ? root.hoverColor : "transparent")
    }

    contentItem: Item {
        anchors.fill: parent

        Column {
            width: parent.width
            anchors.centerIn: parent
            spacing: 2

            Item {
                width: 26
                height: 26
                anchors.horizontalCenter: parent.horizontalCenter

                // Hide the original image; use ColorOverlay for tinting.
                Image {
                    id: iconImage
                    anchors.fill: parent
                    source: root.iconSource
                    fillMode: Image.PreserveAspectFit
                    visible: false
                    smooth: true
                    antialiasing: true
                }

                ColorOverlay {
                    anchors.fill: iconImage
                    source: iconImage
                    color: root.iconColor
                    visible: iconImage.status === Image.Ready
                }

                // Fallback: show the first letter if the icon fails to load.
                Text {
                    anchors.centerIn: parent
                    visible: iconImage.status !== Image.Ready
                    text: (root.text && root.text.length > 0) ? root.text[0] : "?"
                    color: root.textColor
                    font.pixelSize: 14
                }
            }

            Text {
                text: root.text
                color: root.textColor
                font.pixelSize: 10
                horizontalAlignment: Text.AlignHCenter
                width: parent.width
                anchors.horizontalCenter: parent.horizontalCenter
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
        }
    }

    ToolTip.visible: root.hovered && root.tooltipText !== ""
    ToolTip.text: root.tooltipText
    ToolTip.delay: 500

    // Accessibility.
    Accessible.name: root.text
    Accessible.description: root.tooltipText
}
