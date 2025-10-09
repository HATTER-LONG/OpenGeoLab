/**
 * @file BarButton.qml
 * @brief Reusable window bar button component with icon support
 *
 * This component provides a customizable button suitable for window title bars
 * or toolbar areas. Features include:
 * - Icon-based content with automatic sizing
 * - Hover and press state visual feedback
 * - Compact design optimized for toolbars
 * - Mipmap filtering for smooth icon scaling
 */

import QtQuick
import QtQuick.Controls

Button {
    id: root

    // Size configuration: width is 1.5x height for optimal appearance
    width: height * 1.5

    // Remove all default padding and insets for compact appearance
    leftPadding: 0
    topPadding: 0
    rightPadding: 0
    bottomPadding: 0
    leftInset: 0
    topInset: 0
    rightInset: 0
    bottomInset: 0

    /**
     * @brief Source URL for the button icon
     *
     * Should point to an SVG or image file. The icon will be automatically
     * sized and centered within the button.
     */
    property alias source: image.source

    // Content: Centered icon image
    contentItem: Item {
        Image {
            id: image
            anchors.centerIn: parent
            mipmap: true  // Enable mipmap filtering for smoother scaling
            width: 12
            height: 12
            fillMode: Image.PreserveAspectFit
        }
    }

    // Background: Responds to interaction states
    background: Rectangle {
        color: {
            // Disabled state: gray background
            if (!root.enabled) {
                return "gray";
            }
            // Pressed state: semi-transparent black overlay
            if (root.pressed) {
                return Qt.rgba(0, 0, 0, 0.15);
            }
            // Hovered state: semi-transparent black overlay
            if (root.hovered) {
                return Qt.rgba(0, 0, 0, 0.15);
            }
            // Default state: transparent background
            return "transparent";
        }
    }
}
