pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

/**
 * @file RibbonGroupSeparator.qml
 * @brief A vertical separator for use within Ribbon groups
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
