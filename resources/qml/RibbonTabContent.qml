pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

/**
 * @brief Dynamic tab content component that renders buttons from configuration
 *
 * This component reads button configurations and dynamically creates
 * button groups from the configuration data.
 */
Item {
    id: tabContent

    // Configuration for this tab (array of groups)
    property var groups: []

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
                    color: "#666666"
                }

                // Right separator line
                Rectangle {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.topMargin: 5
                    anchors.bottomMargin: 5
                    width: 1
                    color: "#D1D1D1"
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

        RibbonGroupSeparator {}
    }
}
