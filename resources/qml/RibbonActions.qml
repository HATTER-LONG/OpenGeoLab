pragma ComponentBehavior: Bound
import QtQuick

// Non-visual container for ribbon action definitions so they stay in QML.
QtObject {
    // Default actions can be reused by passing this list into RibbonMenu.actions
    // or extended by concatenating additional entries.
    property var defaultActions: [
        {
            id: "file",
            label: "File",
            onTriggered: function () {
                console.log("File menu clicked");
            }
        },
        {
            id: "edit",
            label: "Edit",
            onTriggered: function () {
                console.log("Edit menu clicked");
            }
        },
        {
            id: "view",
            label: "View",
            onTriggered: function () {
                console.log("View menu clicked");
            }
        },
        {
            id: "help",
            label: "Help",
            onTriggered: function () {
                console.log("Help menu clicked");
            }
        }
    ]
}
