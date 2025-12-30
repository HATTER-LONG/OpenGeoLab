pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

/**
 * @brief Base dialog component with standard header, content area, and action buttons.
 *
 * Provides:
 * - Styled header with title and close button
 * - Content area for derived dialogs (auto-sizing with scroll)
 * - Standard OK/Cancel buttons with customizable text
 * - Consistent theming for light/dark modes
 *
 * @note Subclasses should place their UI inside the default content property.
 */
Item {
    id: root

    // Dialog title displayed in the header.
    property string title: ""

    // Button visibility and text customization.
    property bool showOkButton: true
    property bool showCancelButton: true
    property string okButtonText: qsTr("OK")
    property string cancelButtonText: qsTr("Cancel")

    // Enable/disable OK button (useful for validation).
    property bool okEnabled: true

    // Content area: subclasses place their UI here.
    default property alias content: contentArea.data

    // Signals for button actions.
    signal accepted
    signal rejected
    signal closeRequested

    // Auto-sizing: width fixed, height adapts to content with max limit.
    implicitWidth: 520
    implicitHeight: Math.min(mainLayout.implicitHeight, 600)

    // Main container with shadow effect.
    Rectangle {
        id: dialogCard
        anchors.fill: parent
        color: Theme.surfaceColor
        radius: 10

        layer.enabled: true
        layer.effect: DropShadow {
            horizontalOffset: 0
            verticalOffset: 4
            radius: 16
            samples: 33
            color: Qt.rgba(0, 0, 0, 0.25)
        }
    }

    // Border overlay (separate to avoid shadow clipping).
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        radius: 10
        border.width: 1
        border.color: Theme.borderColor
    }

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: 0
        spacing: 0

        // =====================================================================
        // Header
        // =====================================================================
        Rectangle {
            id: header
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: Theme.primaryColor
            radius: 10

            // Square off bottom corners.
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 10
                color: parent.color
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 8
                spacing: 8

                Label {
                    Layout.fillWidth: true
                    text: root.title
                    color: "#FFFFFF"
                    font.pixelSize: 16
                    font.bold: true
                    elide: Text.ElideRight
                }

                // Close button.
                AbstractButton {
                    id: closeBtn
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    hoverEnabled: true

                    background: Rectangle {
                        radius: 4
                        color: closeBtn.pressed ? Qt.rgba(1, 1, 1, 0.3) : (closeBtn.hovered ? Qt.rgba(1, 1, 1, 0.15) : "transparent")
                    }

                    contentItem: Text {
                        text: "\u2715"
                        color: "#FFFFFF"
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        root.rejected();
                        root.closeRequested();
                    }
                }
            }
        }

        // =====================================================================
        // Content Area (scrollable for overflow)
        // =====================================================================
        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 16
            Layout.minimumHeight: 100
            Layout.maximumHeight: 400

            clip: true
            contentWidth: availableWidth

            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            Item {
                id: contentArea
                width: scrollView.availableWidth
                implicitHeight: childrenRect.height
            }
        }

        // =====================================================================
        // Footer with action buttons
        // =====================================================================
        Rectangle {
            id: footer
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: Theme.surfaceAltColor
            radius: 10

            // Square off top corners.
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 10
                color: parent.color
            }

            // Separator line.
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 1
                color: Theme.borderColor
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    id: cancelButton
                    visible: root.showCancelButton
                    text: root.cancelButtonText

                    background: Rectangle {
                        implicitWidth: 90
                        implicitHeight: 36
                        radius: 6
                        color: cancelButton.pressed ? Theme.buttonHoverColor : (cancelButton.hovered ? Qt.lighter(Theme.surfaceAltColor, 1.1) : "transparent")
                        border.width: 1
                        border.color: Theme.borderColor
                    }

                    contentItem: Text {
                        text: cancelButton.text
                        color: Theme.textPrimaryColor
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        root.rejected();
                        root.closeRequested();
                    }
                }

                Button {
                    id: okButton
                    visible: root.showOkButton
                    enabled: root.okEnabled
                    text: root.okButtonText

                    background: Rectangle {
                        implicitWidth: 90
                        implicitHeight: 36
                        radius: 6
                        color: !okButton.enabled ? Theme.buttonDisabledBackgroundColor : (okButton.pressed ? Theme.buttonPressedColor : (okButton.hovered ? Theme.buttonHoverColor : Theme.buttonBackgroundColor))
                    }

                    contentItem: Text {
                        text: okButton.text
                        color: okButton.enabled ? Theme.buttonTextColor : Theme.buttonDisabledTextColor
                        font.pixelSize: 13
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: root.accepted()
                }
            }
        }
    }
}
