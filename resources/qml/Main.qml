pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import OpenGeoLab 1.0 as OGL
import "RibbonMenu" as RibbonMenu
import "Pages" as Pages

/**
 * @file Main.qml
 * @brief Main application window
 */
ApplicationWindow {
    id: root
    visible: true
    width: 1920
    height: 1080
    title: qsTr("OpenGeoLab")

    palette: Theme.palette

    // Store context property reference for ComponentBehavior: Bound
    required property var logService

    // Set main window reference for floating pages
    Component.onCompleted: {
        MainPages.mainWindow = root;
    }

    header: RibbonMenu.RibbonToolBar {
        id: ribbonToolBar
        onActionTriggered: (actionId, payload) => {
            MainPages.handleAction(actionId, payload);
        }
    }

    // Hello World
    Label {
        anchors.centerIn: parent
        text: qsTr("Hello, OpenGeoLab!")
        font.pixelSize: 24
    }

    Pages.CornerOverlay {
        id: cornerOverlay
        logService: root.logService
    }

    // Container for floating function pages
    Item {
        id: functionPagesContainer
        anchors.fill: parent
        z: 100
    }
}
