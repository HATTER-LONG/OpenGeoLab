pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

/**
 * @file ToolDialog.qml
 * @brief Non-modal tool dialog base component
 *
 * Provides a consistent non-modal dialog interface for geometry operations.
 * Features:
 * - Non-modal behavior allowing viewport interaction
 * - Positioned to the right of the model tree sidebar
 * - Close button with hover effect
 * - Standard OK/Cancel buttons with customization
 * - Automatic sizing based on content
 *
 * @note This replaces the modal BaseDialog for tools that need viewport interaction.
 */
Item {
    id: root

    /**
     * @brief Dialog title displayed in the header
     */
    property string title: ""

    /**
     * @brief Show/hide OK button
     */
    property bool showOkButton: true

    /**
     * @brief Show/hide Cancel button
     */
    property bool showCancelButton: true

    /**
     * @brief Text for OK button
     */
    property string okButtonText: qsTr("OK")

    /**
     * @brief Text for Cancel button
     */
    property string cancelButtonText: qsTr("Cancel")

    /**
     * @brief Enable/disable OK button (useful for validation)
     */
    property bool okEnabled: true

    /**
     * @brief Content area: subclasses place their UI here
     */
    default property alias content: contentArea.data

    /**
     * @brief Initial parameters passed to the dialog
     */
    property var initialParams: ({})

    /**
     * @brief Whether the dialog is visible
     */
    property bool isVisible: false

    /**
     * @brief Reference to the viewport for selection integration
     */
    property var viewport: null

    /**
     * @brief Emitted when OK button is clicked
     */
    signal accepted

    /**
     * @brief Emitted when Cancel button is clicked or dialog is closed
     */
    signal rejected

    /**
     * @brief Emitted when close is requested
     */
    signal closeRequested

    visible: isVisible
    width: 360
    height: Math.min(mainLayout.implicitHeight + 100, 10000)

    // Main container with shadow
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
            color: Qt.rgba(0, 0, 0, 0.2)
        }
    }

    // Border overlay
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
        // Header with close button
        // =====================================================================
        Rectangle {
            id: header
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: Theme.primaryColor
            radius: 10

            // Square off bottom corners
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
                    font.pixelSize: 15
                    font.bold: true
                    elide: Text.ElideRight
                }

                // Close button with hover effect
                AbstractButton {
                    id: closeBtn
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    hoverEnabled: true

                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Close")
                    ToolTip.delay: 500

                    background: Rectangle {
                        radius: 4
                        color: closeBtn.pressed ? Qt.rgba(1, 1, 1, 0.3) : (closeBtn.hovered ? Theme.errorColor : "transparent")
                        opacity: closeBtn.hovered ? 0.8 : 1.0

                        Behavior on color {
                            ColorAnimation {
                                duration: 150
                            }
                        }
                    }

                    contentItem: Item {
                        // Close icon (X)
                        Canvas {
                            anchors.centerIn: parent
                            width: 12
                            height: 12
                            onPaint: {
                                var ctx = getContext("2d");
                                ctx.clearRect(0, 0, width, height);
                                ctx.strokeStyle = "#FFFFFF";
                                ctx.lineWidth = 2;
                                ctx.lineCap = "round";
                                ctx.beginPath();
                                ctx.moveTo(2, 2);
                                ctx.lineTo(width - 2, height - 2);
                                ctx.moveTo(width - 2, 2);
                                ctx.lineTo(2, height - 2);
                                ctx.stroke();
                            }
                        }
                    }

                    onClicked: {
                        root.rejected();
                        root.closeRequested();
                    }
                }
            }
        }

        // =====================================================================
        // Content Area (scrollable)
        // =====================================================================
        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 16
            Layout.minimumHeight: 80
            Layout.maximumHeight: 400

            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            Item {
                id: contentArea
                width: scrollView.width
                implicitHeight: childrenRect.height
            }
        }

        // =====================================================================
        // Button Row
        // =====================================================================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: Theme.surfaceAltColor
            radius: 10

            // Square off top corners
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 10
                color: parent.color
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Item {
                    Layout.fillWidth: true
                }

                // Cancel button
                Button {
                    id: cancelBtn
                    visible: root.showCancelButton
                    text: root.cancelButtonText
                    onClicked: {
                        root.rejected();
                        root.closeRequested();
                    }

                    background: Rectangle {
                        implicitWidth: 80
                        implicitHeight: 32
                        color: cancelBtn.hovered ? Theme.surfaceColor : "transparent"
                        border.width: 1
                        border.color: Theme.borderColor
                        radius: 6

                        Behavior on color {
                            ColorAnimation {
                                duration: 100
                            }
                        }
                    }

                    contentItem: Text {
                        text: cancelBtn.text
                        color: Theme.textPrimaryColor
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                // OK button
                Button {
                    id: okBtn
                    visible: root.showOkButton
                    text: root.okButtonText
                    enabled: root.okEnabled
                    onClicked: {
                        root.accepted();
                    }

                    background: Rectangle {
                        implicitWidth: 80
                        implicitHeight: 32
                        color: okBtn.enabled ? (okBtn.pressed ? Theme.buttonPressedColor : (okBtn.hovered ? Theme.buttonHoverColor : Theme.primaryColor)) : Theme.buttonDisabledBackgroundColor
                        radius: 6

                        Behavior on color {
                            ColorAnimation {
                                duration: 100
                            }
                        }
                    }

                    contentItem: Text {
                        text: okBtn.text
                        color: okBtn.enabled ? "#FFFFFF" : Theme.buttonDisabledTextColor
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }

    /**
     * @brief Show the dialog
     */
    function show() {
        root.isVisible = true;
    }

    /**
     * @brief Hide the dialog
     */
    function hide() {
        root.isVisible = false;
    }
}
