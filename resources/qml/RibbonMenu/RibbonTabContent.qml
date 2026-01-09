/**
 * @file RibbonTabContent.qml
 * @brief Content area for a ribbon tab displaying groups and items
 *
 * Renders button groups with titles and separators based on config data.
 */
pragma ComponentBehavior: Bound
import QtQuick
import OpenGeoLab 1.0

Item {
    id: root
    height: parent.height - 28

    /// Array of group definitions with items
    property var groups: []

    Row {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 6

        Repeater {
            model: root.groups

            Rectangle {
                id: groupBox
                required property var modelData

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
                        color: Theme.palette.text
                        font.pixelSize: 10
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                    }
                }
                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    width: 1
                    color: Theme.border
                    opacity: 0.9
                }
            }
        }
    }
    Component {
        id: buttonComponent

        RibbonLargeButton {
            id: btn

            // Loader injects itemData.
            property var itemData: ({})
            // Defensive defaults.
            iconSource: itemData.iconSource || ""
            text: itemData.text || qsTr("Unnamed")
            tooltipText: itemData.tooltip || ""

            onClicked: {
                // Emit a single action signal for the host to handle.
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
            color: Theme.border
            opacity: 0.9
        }
    }
}
