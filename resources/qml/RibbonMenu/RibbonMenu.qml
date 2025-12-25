pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

Rectangle {
    id: ribbonMenu
    width: parent ? parent.width : implicitWidth
    height: 120

    signal actionTriggered(string actionId, var action)

    property var actions: []
    property Component buttonDelegate: defaultButtonDelegate

    readonly property int contentSpacing: 12

    RowLayout {
        id: contentRow
        anchors.fill: parent
        anchors.margins: 10
        spacing: ribbonMenu.contentSpacing

        Repeater {
            model: ribbonMenu.actions
            delegate: Item {
                id: actionDelegate
                required property var modelData
                required property int index

                property var actionData: modelData
                property int actionIndex: index

                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: groupRow.implicitWidth
                Layout.fillHeight: true

                Row {
                    id: groupRow
                    spacing: ribbonMenu.contentSpacing
                    anchors.verticalCenter: parent.verticalCenter

                    RibbonGroupSeparator {
                        visible: actionDelegate.actionIndex > 0
                        height: parent.height
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Loader {
                        id: buttonLoader
                        sourceComponent: ribbonMenu.buttonDelegate
                        property var contextData: actionDelegate.actionData
                        onLoaded: {
                            if (item && item.hasOwnProperty("modelData")) {
                                item.modelData = contextData;
                            }
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }

        Button {
            id: themeToggle
            Layout.alignment: Qt.AlignVCenter
            text: Theme.mode === Theme.dark ? qsTr("Light Theme") : qsTr("Dark Theme")
            onClicked: Theme.mode = (Theme.mode === Theme.dark) ? Theme.light : Theme.dark

            font.family: Theme.fontFamily
            font.pointSize: Theme.fontPointSizeBase

            contentItem: Text {
                text: themeToggle.text
                color: themeToggle.enabled ? Theme.buttonTextColor : Theme.buttonDisabledTextColor
                font.family: Theme.fontFamily
                font.pointSize: Theme.fontPointSizeBase
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            background: Rectangle {
                radius: 6
                border.width: 1
                border.color: Theme.buttonBorderColor
                color: themeToggle.enabled ? (themeToggle.down ? Theme.buttonPressedColor : (themeToggle.hovered ? Theme.buttonHoverColor : Theme.buttonBackgroundColor)) : Theme.buttonDisabledBackgroundColor
            }
        }
    }

    Component {
        id: defaultButtonDelegate

        Button {
            id: actionButton
            property var modelData

            text: modelData.label ?? ""
            width: 96
            height: 40

            font.family: Theme.fontFamily
            font.pointSize: Theme.fontPointSizeBase

            contentItem: Text {
                text: actionButton.text
                color: actionButton.enabled ? Theme.buttonTextColor : Theme.buttonDisabledTextColor
                font.family: actionButton.font.family
                font.pointSize: actionButton.font.pointSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            background: Rectangle {
                radius: 6
                border.width: 1
                border.color: Theme.buttonBorderColor
                color: actionButton.enabled ? (actionButton.down ? Theme.buttonPressedColor : (actionButton.hovered ? Theme.buttonHoverColor : Theme.buttonBackgroundColor)) : Theme.buttonDisabledBackgroundColor
            }

            onClicked: {
                if (typeof modelData.onTriggered === "function") {
                    modelData.onTriggered(modelData);
                }
                ribbonMenu.actionTriggered(modelData.id ?? "", modelData);
            }
        }
    }
}
