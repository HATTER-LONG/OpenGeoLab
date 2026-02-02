/**
 * @file EntityCountRow.qml
 * @brief Helper component for displaying entity type counts
 *
 * Shows a single row with an icon, label, and count value.
 * Used within PartListItem to display entity statistics.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

RowLayout {
    id: root

    /// Display label for the entity type
    property string label: ""

    /// Number of entities
    property int count: 0

    /// Icon source URL
    property url iconSource: ""

    spacing: 6
    Layout.fillWidth: true

    ThemedIcon {
        source: root.iconSource
        size: 12
        color: Theme.textSecondary
        visible: root.iconSource.toString() !== ""
    }

    Label {
        text: root.label
        font.pixelSize: 11
        color: Theme.textSecondary
        Layout.fillWidth: true
    }

    Label {
        text: root.count.toString()
        font.pixelSize: 11
        font.bold: true
        color: Theme.textPrimary
    }
}
