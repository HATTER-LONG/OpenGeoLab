pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

/**
 * @brief A vertical separator for use within Ribbon groups
 *
 * Color can be customized to match the current theme.
 */
Rectangle {
    width: 1
    height: 60

    Layout.preferredWidth: 1
    Layout.fillHeight: true
    Layout.topMargin: 5
    Layout.bottomMargin: 5
    color: "#3a3f4b"
}
