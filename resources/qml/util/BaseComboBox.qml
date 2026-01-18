pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import ".."

/**
 * @file BaseComboBox.qml
 * @brief Themed ComboBox component with consistent styling
 */
ComboBox {
    id: control

    implicitWidth: 100
    implicitHeight: 24

    delegate: ItemDelegate {
        id: itemDelegate
        required property var modelData
        required property int index

        width: control.width
        height: 28
        padding: 8

        contentItem: Text {
            text: control.textRole ? (itemDelegate.modelData[control.textRole] ?? itemDelegate.modelData) : itemDelegate.modelData
            color: itemDelegate.highlighted ? Theme.white : Theme.palette.text
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            color: itemDelegate.highlighted ? Theme.accent : (itemDelegate.hovered ? Theme.hovered : "transparent")
        }

        highlighted: control.highlightedIndex === itemDelegate.index
    }

    indicator: Text {
        x: control.width - width - 8
        y: (control.height - height) / 2
        text: "▼"
        color: control.palette.text
        font.pixelSize: 8
    }

    contentItem: Text {
        leftPadding: 8
        rightPadding: control.indicator.width + 8
        text: control.displayText
        font.pixelSize: 12
        color: control.palette.text
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: 6
        color: control.down ? Theme.clicked : control.hovered ? Theme.hovered : Theme.surfaceAlt
        border.width: 1
        border.color: Theme.border
    }

    popup: Popup {
        y: control.height + 2
        width: control.width
        implicitHeight: contentItem.implicitHeight + 2
        padding: 1

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex

            ScrollIndicator.vertical: ScrollIndicator {}
        }

        background: Rectangle {
            radius: 6
            color: Theme.surface
            border.width: 1
            border.color: Theme.border
        }
    }
}
