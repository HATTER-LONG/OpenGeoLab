pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import "../theme"

Item {
    id: pageRoot

    required property AppTheme theme
    property bool open: false
    property string pageTitle: qsTr("Feature Page")
    property string sectionTitle: ""
    property string summaryText: ""
    property string statusBadgeText: qsTr("Action")
    property string iconKind: "menu"
    property string accentName: "accentA"
    property string executeButtonText: qsTr("Execute")
    property string cancelButtonText: qsTr("Cancel")
    property bool closeOnExecute: true
    property int maxPanelWidth: 560
    property int maxPanelHeight: 460
    property int minPanelHeight: 320
    property real preferredPanelX: -1
    property real preferredPanelY: -1
    readonly property color accentColor: resolveAccentColor(accentName)
    readonly property int panelMargin: theme.shellPadding
    readonly property int availablePanelWidth: Math.max(240, width - panelMargin * 2)
    readonly property int availablePanelHeight: Math.max(220, height - panelMargin * 2)
    readonly property int panelWidth: Math.min(maxPanelWidth, availablePanelWidth)
    readonly property int preferredPanelHeight: toolbarBar.implicitHeight + footerPanel.implicitHeight + theme.shellPadding * 2 + theme.gap + contentColumn.implicitHeight
    readonly property int panelHeight: Math.min(maxPanelHeight, availablePanelHeight, Math.max(minPanelHeight, preferredPanelHeight))
    property real panelX: Math.round((width - panelWidth) / 2)
    property real panelY: Math.max(panelMargin, Math.round((height - panelHeight) / 2))
    default property alias contentData: contentColumn.data
    signal executeRequested
    signal cancelRequested

    visible: open
    z: 200

    function clampPanelPosition() {
        const maxX = Math.max(panelMargin, width - panelWidth - panelMargin);
        const maxY = Math.max(panelMargin, height - panelHeight - panelMargin);
        panelX = Math.max(panelMargin, Math.min(maxX, panelX));
        panelY = Math.max(panelMargin, Math.min(maxY, panelY));
    }

    function resolveAccentColor(name) {
        if (name === "accentB") {
            return theme.accentB;
        }
        if (name === "accentC") {
            return theme.accentC;
        }
        if (name === "accentD") {
            return theme.accentD;
        }
        if (name === "accentE") {
            return theme.accentE;
        }
        return theme.accentA;
    }

    function present() {
        panelX = preferredPanelX >= panelMargin ? preferredPanelX : Math.round((width - panelWidth) / 2);
        panelY = preferredPanelY >= panelMargin ? preferredPanelY : Math.max(panelMargin, Math.round((height - panelHeight) / 2));
        clampPanelPosition();
        open = true;
    }

    function execute() {
        executeRequested();
        if (closeOnExecute) {
            open = false;
        }
    }

    function cancel() {
        cancelRequested();
        open = false;
    }

    Rectangle {
        id: panelSurface

        x: pageRoot.panelX
        y: pageRoot.panelY
        width: pageRoot.panelWidth
        height: pageRoot.panelHeight
        radius: pageRoot.theme.radiusLarge
        color: pageRoot.theme.tint(pageRoot.theme.surface, pageRoot.theme.darkMode ? 0.98 : 1.0)
        border.width: 1
        border.color: pageRoot.theme.tint(pageRoot.accentColor, pageRoot.theme.darkMode ? 0.38 : 0.2)

        onWidthChanged: pageRoot.clampPanelPosition()
        onHeightChanged: pageRoot.clampPanelPosition()
    }

    ColumnLayout {
        anchors.fill: panelSurface
        spacing: 0

        Item {
            id: toolbarBar

            Layout.fillWidth: true
            implicitHeight: 36

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: pageRoot.theme.tint(pageRoot.accentColor, pageRoot.theme.darkMode ? 0.34 : 0.18)
            }

            MouseArea {
                id: toolbarDragArea

                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                hoverEnabled: true
                cursorShape: Qt.SizeAllCursor
                property real lastMouseX: 0
                property real lastMouseY: 0

                onPressed: function (mouse) {
                    toolbarDragArea.lastMouseX = mouse.x;
                    toolbarDragArea.lastMouseY = mouse.y;
                }
                onPositionChanged: function (mouse) {
                    if (!toolbarDragArea.pressed) {
                        return;
                    }

                    pageRoot.panelX += mouse.x - toolbarDragArea.lastMouseX;
                    pageRoot.panelY += mouse.y - toolbarDragArea.lastMouseY;
                    pageRoot.clampPanelPosition();
                }
            }

            RowLayout {
                id: toolbarRow

                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 10
                anchors.topMargin: 3
                anchors.bottomMargin: 3
                spacing: 8

                Rectangle {
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    radius: 7
                    color: pageRoot.theme.tint(pageRoot.accentColor, pageRoot.theme.darkMode ? 0.18 : 0.1)
                    border.width: 1
                    border.color: pageRoot.theme.tint(pageRoot.accentColor, pageRoot.theme.darkMode ? 0.42 : 0.22)

                    AppIcon {
                        anchors.centerIn: parent
                        width: 12
                        height: 12
                        theme: pageRoot.theme
                        iconKind: pageRoot.iconKind
                        primaryColor: pageRoot.theme.textPrimary
                    }
                }

                Text {
                    text: pageRoot.pageTitle
                    color: pageRoot.theme.textPrimary
                    font.pixelSize: 12
                    font.bold: true
                    font.family: pageRoot.theme.bodyFontFamily
                    elide: Text.ElideRight
                    Layout.maximumWidth: 220
                }

                Text {
                    text: pageRoot.sectionTitle
                    color: pageRoot.theme.textSecondary
                    font.pixelSize: 10
                    font.family: pageRoot.theme.bodyFontFamily
                    elide: Text.ElideRight
                    Layout.maximumWidth: 150
                }

                Item {
                    Layout.fillWidth: true
                }

                Rectangle {
                    Layout.preferredWidth: 22
                    Layout.preferredHeight: 22
                    radius: 7
                    color: closeMouseArea.pressed ? pageRoot.theme.tint(pageRoot.theme.surfaceStrong, pageRoot.theme.darkMode ? 0.94 : 0.98) : (closeMouseArea.containsMouse ? pageRoot.theme.tint(pageRoot.theme.surfaceStrong, pageRoot.theme.darkMode ? 0.82 : 0.92) : "transparent")
                    border.width: closeMouseArea.containsMouse ? 1 : 0
                    border.color: pageRoot.theme.tint(pageRoot.theme.borderSubtle, 0.82)

                    Text {
                        anchors.centerIn: parent
                        text: "\u00d7"
                        color: pageRoot.theme.textPrimary
                        font.pixelSize: 15
                        font.bold: true
                        font.family: pageRoot.theme.bodyFontFamily
                    }

                    MouseArea {
                        id: closeMouseArea

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: pageRoot.cancel()
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: pageRoot.theme.shellPadding
                spacing: pageRoot.theme.gap

                Flickable {
                    id: pageScroller

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: width
                    contentHeight: contentColumn.implicitHeight
                    boundsBehavior: Flickable.StopAtBounds

                    ColumnLayout {
                        id: contentColumn

                        width: pageScroller.width
                        spacing: pageRoot.theme.gap
                    }
                }

                Rectangle {
                    id: footerPanel

                    Layout.fillWidth: true
                    radius: pageRoot.theme.radiusMedium
                    color: pageRoot.theme.tint(pageRoot.theme.surfaceMuted, pageRoot.theme.darkMode ? 0.56 : 0.9)
                    border.width: 1
                    border.color: pageRoot.theme.tint(pageRoot.theme.borderSubtle, 0.78)
                    implicitHeight: footerRow.implicitHeight + 16

                    RowLayout {
                        id: footerRow

                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8

                        Item {
                            Layout.fillWidth: true
                        }

                        ActionButton {
                            Layout.preferredWidth: 104
                            theme: pageRoot.theme
                            buttonText: pageRoot.executeButtonText
                            buttonColor: pageRoot.theme.tint(pageRoot.accentColor, pageRoot.theme.darkMode ? 0.22 : 0.12)
                            pressedColor: pageRoot.theme.tint(pageRoot.accentColor, pageRoot.theme.darkMode ? 0.32 : 0.18)
                            hoverBorderColor: pageRoot.theme.tint(pageRoot.accentColor, pageRoot.theme.darkMode ? 0.58 : 0.34)
                            onClicked: pageRoot.execute()
                        }

                        ActionButton {
                            Layout.preferredWidth: 104
                            theme: pageRoot.theme
                            buttonText: pageRoot.cancelButtonText
                            buttonColor: pageRoot.theme.tint(pageRoot.theme.surfaceMuted, pageRoot.theme.darkMode ? 0.7 : 0.96)
                            pressedColor: pageRoot.theme.tint(pageRoot.theme.surfaceStrong, pageRoot.theme.darkMode ? 0.88 : 0.98)
                            hoverBorderColor: pageRoot.theme.tint(pageRoot.theme.borderSubtle, pageRoot.theme.darkMode ? 0.8 : 0.54)
                            quiet: true
                            onClicked: pageRoot.cancel()
                        }
                    }
                }
            }
        }
    }

    onWidthChanged: clampPanelPosition()
    onHeightChanged: clampPanelPosition()
    onPanelWidthChanged: clampPanelPosition()
    onPanelHeightChanged: clampPanelPosition()
}
