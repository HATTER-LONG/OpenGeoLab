pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * @brief Minimal file menu popup.
 *
 * Notes:
 * - Keeps a `setRecentFiles()` API for compatibility, but the UI intentionally
 *   does not show a Recent section (requested simplification).
 */
Popup {
    id: root

    signal actionTriggered(string actionId, var payload)

    width: 260
    implicitHeight: contentColumn.implicitHeight + padding * 2
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 14

    property var recentFiles: []

    readonly property int _menuRowHeight: 38

    function trigger(actionId, payload): void {
        root.close();
        root.actionTriggered(actionId, payload);
    }

    background: Rectangle {
        radius: 6
        color: Theme.ribbonPopupBackgroundColor
        border.color: Theme.ribbonBorderColor
        border.width: 1
    }

    contentItem: ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        spacing: 10

        readonly property var _primaryItems: ([
                {
                    text: qsTr("New"),
                    id: "newFile"
                },
                {
                    text: qsTr("Open"),
                    id: "openFile"
                },
                {
                    text: qsTr("Import"),
                    id: "importModel"
                },
                {
                    text: qsTr("Export"),
                    id: "exportModel"
                }
            ])

        readonly property var _secondaryItems: ([
                {
                    text: qsTr("Toggle Theme"),
                    id: "toggleTheme"
                },
                {
                    text: qsTr("Exit"),
                    id: "exitApp"
                }
            ])

        Repeater {
            model: contentColumn._primaryItems

            Rectangle {
                id: primaryRow
                required property var modelData

                Layout.fillWidth: true
                height: root._menuRowHeight
                radius: 8
                color: mouseArea.pressed ? Theme.ribbonPressedColor : (mouseArea.containsMouse ? Theme.ribbonHoverColor : "transparent")

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: primaryRow.modelData.text
                    color: Theme.ribbonTextColor
                    font.pixelSize: 13
                    elide: Text.ElideRight
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.trigger(primaryRow.modelData.id, null)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.ribbonBorderColor
            opacity: 0.7
        }

        Repeater {
            model: contentColumn._secondaryItems

            Rectangle {
                id: secondaryRow
                required property var modelData

                Layout.fillWidth: true
                height: root._menuRowHeight
                radius: 8
                color: mouseArea2.pressed ? Theme.ribbonPressedColor : (mouseArea2.containsMouse ? Theme.ribbonHoverColor : "transparent")

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: secondaryRow.modelData.text
                    color: Theme.ribbonTextColor
                    font.pixelSize: 13
                    elide: Text.ElideRight
                }

                MouseArea {
                    id: mouseArea2
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.trigger(secondaryRow.modelData.id, null)
                }
            }
        }
    }

    function setRecentFiles(files): void {
        root.recentFiles = files || [];
    }
}
