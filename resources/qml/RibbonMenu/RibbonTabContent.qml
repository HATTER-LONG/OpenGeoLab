pragma ComponentBehavior: Bound
import QtQuick

Item {
    id: root

    property var groups: []

    property color iconColor: Theme.ribbonIconColor
    property color textColor: Theme.ribbonTextColor
    property color textColorDim: Theme.ribbonTextDimColor
    property color hoverColor: Theme.ribbonHoverColor
    property color pressedColor: Theme.ribbonPressedColor
    property color separatorColor: Theme.ribbonBorderColor

    signal actionTriggered(string actionId, var params)
    Row {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 6

        Repeater {
            model: root.groups

            Rectangle {
                id: groupBox
                required property var modelData

                // Transparent layout container for a group.
                color: "transparent"
                height: parent ? parent.height : 90
                implicitWidth: contentRow.implicitWidth + 14

                Column {
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 2

                    Item {
                        id: contentHost
                        width: parent.width
                        height: parent.height - titleLabel.implicitHeight - 2

                        // Button row centered in the content host.
                        Row {
                            id: contentRow
                            anchors.centerIn: parent
                            spacing: 4

                            Repeater {
                                model: (groupBox.modelData.items || [])

                                Loader {
                                    required property var modelData

                                    // Select a component by item type.
                                    sourceComponent: {
                                        const t = (modelData.type || "button");
                                        if (t === "separator")
                                            return separatorComponent;
                                        return buttonComponent;
                                    }

                                    onLoaded: {
                                        // Inject modelData into the created item (button/separator).
                                        if (item && item.hasOwnProperty("itemData")) {
                                            item.itemData = modelData;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Text {
                        id: titleLabel
                        width: contentRow.width
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: groupBox.modelData.title || ""
                        color: root.textColorDim
                        font.pixelSize: 10
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                    }
                }

                // Right-side separator between groups.
                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    width: 1
                    color: root.separatorColor
                    opacity: 0.9
                }
            }
        }
    }

    // ===== Button template =====
    Component {
        id: buttonComponent

        RibbonLargeButton {
            id: btn

            // Loader injects itemData.
            property var itemData: ({})
            iconColor: root.iconColor
            textColor: root.textColor
            hoverColor: root.hoverColor
            pressedColor: root.pressedColor

            // Defensive defaults.
            iconSource: itemData.iconSource || ""
            text: itemData.text || qsTr("Unnamed")
            tooltipText: itemData.tooltip || ""

            onClicked: {
                // Emit a single action signal for the host to handle.
                root.actionTriggered(itemData.id || "", itemData.payload);
            }
        }
    }

    // ===== Separator template =====
    Component {
        id: separatorComponent

        Rectangle {
            // Keep itemData for structural symmetry with the button delegate.
            property var itemData: ({})
            width: 1
            height: 60
            color: root.separatorColor
            opacity: 0.9
        }
    }
}
