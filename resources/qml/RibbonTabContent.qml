pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

/**
 * @file RibbonTabContent.qml
 * @brief Dynamic tab content component that renders buttons from configuration
 *
 * This component reads button configurations and dynamically creates
 * button groups. Colors are inherited from parent RibbonToolBar.
 */
Item {
    id: tabContent

    // Configuration for this tab (array of groups)
    property var groups: []

    // Dark theme colors - can be inherited from parent RibbonToolBar
    property color iconColor: "#e1e1e1"
    property color textColor: "#ffffff"
    property color textColorDim: "#b8b8b8"
    property color hoverColor: "#3a3f4b"
    property color pressedColor: "#4a5568"
    property color separatorColor: "#3a3f4b"

    // Signal emitted when any button is clicked, with the button's action ID
    signal buttonClicked(string actionId)

    Row {
        anchors.fill: parent
        anchors.margins: 5
        spacing: 2

        Repeater {
            id: groupRepeater
            model: tabContent.groups

            // Each group is a Rectangle container
            Rectangle {
                id: groupContainer
                required property var modelData
                required property int index

                implicitWidth: buttonRow.width + 16
                height: parent ? parent.height : 100
                color: "transparent"

                // Buttons row
                Row {
                    id: buttonRow
                    anchors.top: parent.top
                    anchors.topMargin: 2
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 4

                    Repeater {
                        model: groupContainer.modelData.buttons || []

                        Loader {
                            id: buttonLoader
                            required property var modelData
                            required property int index

                            sourceComponent: {
                                if (modelData.type === "separator") {
                                    return separatorComponent;
                                }
                                return buttonComponent;
                            }

                            onLoaded: {
                                if (item && modelData.type !== "separator") {
                                    item.buttonData = modelData;
                                    // Pass theme colors to button
                                    item.iconColor = tabContent.iconColor;
                                    item.textColor = tabContent.textColor;
                                    item.hoverColor = tabContent.hoverColor;
                                    item.pressedColor = tabContent.pressedColor;
                                }
                            }
                        }
                    }
                }

                // Title at bottom
                Text {
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 4
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: groupContainer.modelData.title || ""
                    font.pixelSize: 10
                    color: tabContent.textColorDim
                }

                // Right separator line
                Rectangle {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.topMargin: 5
                    anchors.bottomMargin: 5
                    width: 1
                    color: tabContent.separatorColor
                }
            }
        }
    }

    // Button component template
    Component {
        id: buttonComponent

        RibbonLargeButton {
            id: btn
            property var buttonData: ({})

            iconSource: buttonData.iconSource || ""
            text: buttonData.text || ""

            onClicked: {
                if (buttonData.id) {
                    tabContent.buttonClicked(buttonData.id);
                }
            }
        }
    }

    // Separator component template
    Component {
        id: separatorComponent

        RibbonGroupSeparator {
            color: tabContent.separatorColor
        }
    }
}
