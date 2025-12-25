pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Material
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
            onClicked: Theme.mode = Theme.mode === Theme.dark ? Theme.light : Theme.dark
        }
    }

    Component {
        id: defaultButtonDelegate

        Button {
            property var modelData

            text: modelData.label ?? ""
            width: 96
            height: 40

            onClicked: {
                if (typeof modelData.onTriggered === "function") {
                    modelData.onTriggered(modelData);
                }
                ribbonMenu.actionTriggered(modelData.id ?? "", modelData);
            }
        }
    }
}
