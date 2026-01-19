pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import OpenGeoLab 1.0
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
        logService: LogService
    }
}
