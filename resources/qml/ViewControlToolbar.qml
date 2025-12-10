/**
 * ViewControlToolbar.qml
 * Provides view control buttons for 3D rendering
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    // Target geometry renderer
    property var targetRenderer: null

    // Appearance
    color: Qt.rgba(0.15, 0.15, 0.18, 0.9)
    radius: 8
    border.color: Qt.rgba(0.3, 0.3, 0.35, 1.0)
    border.width: 1

    implicitWidth: buttonLayout.implicitWidth + 20
    implicitHeight: buttonLayout.implicitHeight + 16

    RowLayout {
        id: buttonLayout
        anchors.centerIn: parent
        spacing: 4

        // Zoom In button
        ViewControlButton {
            iconSource: "qrc:/scenegraph/opengeolab/resources/icons/zoom-in.svg"
            tooltipText: qsTr("Zoom In")
            onClicked: {
                if (root.targetRenderer) {
                    root.targetRenderer.zoomIn(1.2);
                }
            }
        }

        // Zoom Out button
        ViewControlButton {
            iconSource: "qrc:/scenegraph/opengeolab/resources/icons/zoom-out.svg"
            tooltipText: qsTr("Zoom Out")
            onClicked: {
                if (root.targetRenderer) {
                    root.targetRenderer.zoomOut(1.2);
                }
            }
        }

        // Separator
        Rectangle {
            width: 1
            height: 24
            color: Qt.rgba(0.4, 0.4, 0.45, 1.0)
            Layout.leftMargin: 4
            Layout.rightMargin: 4
        }

        // Fit to View button
        ViewControlButton {
            iconSource: "qrc:/scenegraph/opengeolab/resources/icons/fit-view.svg"
            tooltipText: qsTr("Fit to View")
            onClicked: {
                if (root.targetRenderer) {
                    root.targetRenderer.fitToView();
                }
            }
        }

        // Reset View button
        ViewControlButton {
            iconSource: "qrc:/scenegraph/opengeolab/resources/icons/reset-view.svg"
            tooltipText: qsTr("Reset View")
            onClicked: {
                if (root.targetRenderer) {
                    root.targetRenderer.resetView();
                }
            }
        }

        // Separator
        Rectangle {
            width: 1
            height: 24
            color: Qt.rgba(0.4, 0.4, 0.45, 1.0)
            Layout.leftMargin: 4
            Layout.rightMargin: 4
        }

        // Standard Views dropdown
        ViewControlButton {
            id: viewsButton
            iconSource: "qrc:/scenegraph/opengeolab/resources/icons/cube-view.svg"
            tooltipText: qsTr("Standard Views")
            hasDropdown: true
            onClicked: viewsMenu.open()

            Menu {
                id: viewsMenu
                y: viewsButton.height + 4

                MenuItem {
                    text: qsTr("Front")
                    onTriggered: {
                        if (root.targetRenderer) {
                            root.targetRenderer.setViewFront();
                        }
                    }
                }
                MenuItem {
                    text: qsTr("Back")
                    onTriggered: {
                        if (root.targetRenderer) {
                            root.targetRenderer.setViewBack();
                        }
                    }
                }
                MenuItem {
                    text: qsTr("Top")
                    onTriggered: {
                        if (root.targetRenderer) {
                            root.targetRenderer.setViewTop();
                        }
                    }
                }
                MenuItem {
                    text: qsTr("Bottom")
                    onTriggered: {
                        if (root.targetRenderer) {
                            root.targetRenderer.setViewBottom();
                        }
                    }
                }
                MenuItem {
                    text: qsTr("Left")
                    onTriggered: {
                        if (root.targetRenderer) {
                            root.targetRenderer.setViewLeft();
                        }
                    }
                }
                MenuItem {
                    text: qsTr("Right")
                    onTriggered: {
                        if (root.targetRenderer) {
                            root.targetRenderer.setViewRight();
                        }
                    }
                }
                MenuSeparator {}
                MenuItem {
                    text: qsTr("Isometric")
                    onTriggered: {
                        if (root.targetRenderer) {
                            root.targetRenderer.setViewIsometric();
                        }
                    }
                }
            }
        }
    }
}
