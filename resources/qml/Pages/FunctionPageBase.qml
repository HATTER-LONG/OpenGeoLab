/**
 * @file FunctionPageBase.qml
 * @brief Base component for floating, draggable, non-modal function pages
 *
 * Provides a reusable floating panel with:
 * - Draggable title bar with icon and title
 * - Parameter content area (to be filled by subclasses)
 * - Execute and Cancel action buttons
 * - JSON request emission on execute
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0
import ".."

Item {
    id: root

    // =========================================================
    // Public API
    // =========================================================

    /// Page title displayed in header
    property string pageTitle: qsTr("Function")
    /// Page icon source URL
    property string pageIcon: ""
    /// Service name for backend request
    property string serviceName: ""
    /// Unique action identifier
    property string actionId: ""

    /// Opt-in: whether this page participates in viewport picking via PickManager
    property bool usesPicking: false

    /// Whether the page is currently visible
    property bool pageVisible: false

    /// Default content to be overridden by subclasses
    default property alias content: contentColumn.data

    /// Parameters object - subclasses should define their own properties
    /// and build JSON in getParameters()
    property var parameters: ({})

    /// Signal emitted when execute is clicked with JSON payload
    signal executed(string jsonPayload)
    /// Signal emitted when cancel is clicked
    signal cancelled

    // =========================================================
    // Layout
    // =========================================================

    visible: pageVisible
    width: 320
    height: panelColumn.implicitHeight
    z: 1000

    x: 12
    y: 12

    // =========================================================
    // Positioning (avoid overlapping the left sidebar)
    // =========================================================

    /// Gap between sidebar and floating page
    property int sidebarGap: 12

    /// Track sidebar width changes so we can shift the page with it
    property real _lastSidebarWidth: 0

    function _getSidebarWidth() {
        const mw = MainPages.mainWindow;
        if (mw && mw.documentSideBar)
            return mw.documentSideBar.width;
        return 0;
    }

    function _clampX(value) {
        const parentWidth = root.parent ? root.parent.width : 0;
        if (parentWidth > 0)
            return Math.max(0, Math.min(parentWidth - root.width, value));
        return Math.max(0, value);
    }

    function _minXRightOfSidebar() {
        return _getSidebarWidth() + sidebarGap;
    }

    function _ensureRightOfSidebar() {
        const minX = _minXRightOfSidebar();
        root.x = _clampX(Math.max(root.x, minX));
    }

    // =========================================================
    // Methods
    // =========================================================

    /**
     * @brief Open the function page
     * @param payload Optional initial data
     */
    function open(payload) {
        if (payload) {
            parsePayload(payload);
        }

        // Position the page to the right of the sidebar on open
        _lastSidebarWidth = _getSidebarWidth();

        var tempx = _clampX(_minXRightOfSidebar());
        if (tempx > root.x) {
            root.x = tempx;
        }

        pageVisible = true;
        root.forceActiveFocus();

        if (usesPicking) {
            PickManager.setActiveConsumer(root.actionId);
        }
    }

    /**
     * @brief Close the function page
     */
    function close() {
        pageVisible = false;

        if (usesPicking && PickManager.activeConsumerKey === root.actionId) {
            PickManager.deactivatePickMode();
            PickManager.clearActiveConsumer();
        }

        // Notify MainPages that this page is closed
        if (MainPages.currentOpenPage === root.actionId) {
            MainPages.currentOpenPage = "";
        }
    }

    /**
     * @brief Override in subclasses to parse incoming payload
     * @param payload The payload data
     */
    function parsePayload(payload) {
        // Override in subclass
    }

    /**
     * @brief Override in subclasses to build parameter JSON
     * @return Object containing parameters for backend request
     */
    function getParameters() {
        return parameters;
    }

    /**
     * @brief Execute the function and send request to backend
     */
    function execute() {
        const params = getParameters();
        const jsonPayload = JSON.stringify(params);
        console.log("[FunctionPage]", actionId, "executing with:", jsonPayload);

        if (serviceName) {
            BackendService.request(serviceName, jsonPayload);
        }
        executed(jsonPayload);
    }

    // =========================================================
    // UI Components
    // =========================================================

    /// Shadow effect
    Rectangle {
        id: shadow
        anchors.fill: panel
        anchors.margins: -2
        radius: panel.radius + 2
        color: "transparent"
        border.width: 0

        Rectangle {
            anchors.fill: parent
            anchors.margins: -4
            radius: parent.radius + 4
            color: Theme.isDark ? Qt.rgba(0, 0, 0, 0.4) : Qt.rgba(0, 0, 0, 0.15)
            z: -1
        }
    }

    /// Main panel container
    Rectangle {
        id: panel
        anchors.fill: parent
        radius: 8
        color: Theme.surface
        border.width: 0.5
        border.color: Theme.border

        Column {
            id: panelColumn
            anchors.fill: parent
            anchors.margins: 1
            spacing: 0

            // =====================================================
            // Title Bar (Draggable)
            // =====================================================
            Rectangle {
                id: titleBar
                width: parent.width
                height: 36
                radius: 7
                color: Theme.ribbonBackground

                // Bottom corners should not be rounded
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: parent.radius
                    color: parent.color
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 6
                    spacing: 8

                    ThemedIcon {
                        source: root.pageIcon
                        size: 18
                        color: Theme.textPrimary
                        visible: root.pageIcon.length > 0
                    }

                    Label {
                        text: root.pageTitle
                        font.pixelSize: 13
                        font.bold: true
                        color: Theme.textPrimary
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    // Close button
                    AbstractButton {
                        id: closeButton
                        implicitWidth: 24
                        implicitHeight: 24
                        hoverEnabled: true

                        background: Rectangle {
                            radius: 4
                            color: closeButton.hovered ? Theme.danger : "transparent"
                        }

                        contentItem: Label {
                            text: "✕"
                            font.pixelSize: 12
                            color: closeButton.hovered ? Theme.white : Theme.textSecondary
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        onClicked: {
                            root.cancelled();
                            root.close();
                        }

                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Close")
                        ToolTip.delay: 500
                    }
                }

                // Drag handler
                MouseArea {
                    id: dragArea
                    anchors.fill: parent
                    anchors.rightMargin: 30 // Leave space for close button
                    cursorShape: Qt.SizeAllCursor
                    property point lastPos

                    onPressed: mouse => {
                        lastPos = Qt.point(mouse.x, mouse.y);
                    }

                    onPositionChanged: mouse => {
                        if (pressed) {
                            const dx = mouse.x - lastPos.x;
                            const dy = mouse.y - lastPos.y;
                            const minX = root._minXRightOfSidebar();
                            root.x = Math.max(minX, Math.min(root.parent.width - root.width, root.x + dx));
                            root.y = Math.max(12, Math.min(root.parent.height - root.height, root.y + dy));
                        }
                    }
                }
            }

            // =====================================================
            // Separator
            // =====================================================
            Rectangle {
                width: parent.width
                height: 1
                color: Theme.border
            }

            // =====================================================
            // Content Area (Parameters)
            // =====================================================
            Item {
                id: contentArea
                width: parent.width
                height: contentColumn.height + 24

                Column {
                    id: contentColumn
                    x: 12
                    y: 12
                    width: parent.width - 24
                    spacing: 12
                }
            }

            // =====================================================
            // Separator
            // =====================================================
            Rectangle {
                width: parent.width
                height: 1
                color: Theme.border
            }

            // =====================================================
            // Action Buttons
            // =====================================================
            Item {
                width: parent.width
                height: 48

                RowLayout {
                    anchors.centerIn: parent
                    anchors.margins: 12
                    spacing: 12

                    // Execute Button
                    Button {
                        id: executeButton
                        text: qsTr("Execute")
                        implicitWidth: 90
                        implicitHeight: 32

                        background: Rectangle {
                            radius: 4
                            color: executeButton.pressed ? Qt.darker(Theme.accent, 1.2) : executeButton.hovered ? Qt.lighter(Theme.accent, 1.1) : Theme.accent
                            border.width: 1
                            border.color: Qt.darker(Theme.accent, 1.3)
                        }

                        contentItem: Label {
                            text: executeButton.text
                            font.pixelSize: 12
                            font.bold: true
                            color: Theme.white
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        onClicked: root.execute()

                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Execute this function")
                        ToolTip.delay: 500
                    }

                    // Cancel Button
                    Button {
                        id: cancelButton
                        text: qsTr("Cancel")
                        implicitWidth: 90
                        implicitHeight: 32

                        background: Rectangle {
                            radius: 4
                            color: cancelButton.pressed ? Theme.clicked : cancelButton.hovered ? Theme.hovered : Theme.surfaceAlt
                            border.width: 1
                            border.color: Theme.border
                        }

                        contentItem: Label {
                            text: cancelButton.text
                            font.pixelSize: 12
                            color: Theme.textPrimary
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        onClicked: {
                            root.cancelled();
                            root.close();
                        }

                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Cancel and close")
                        ToolTip.delay: 500
                    }
                }
            }
        }
    }

    // Keyboard handling
    Keys.onEscapePressed: {
        cancelled();
        close();
    }

    // Follow the sidebar expand/collapse width animation.
    Connections {
        target: (MainPages.mainWindow && MainPages.mainWindow.documentSideBar) ? MainPages.mainWindow.documentSideBar : null

        function onWidthChanged() {
            const sidebarWidth = root._getSidebarWidth();
            const delta = sidebarWidth - root._lastSidebarWidth;
            root._lastSidebarWidth = sidebarWidth;

            if (!root.pageVisible)
                return;

            // Keep the page visually in the same spot relative to the sidebar.
            root.x = root._clampX(root.x + delta);
            root._ensureRightOfSidebar();
        }
    }
}
