pragma ComponentBehavior: Bound

import QtQuick

import "../theme"

Item {
    id: root

    required property AppTheme theme
    required property var pageState

    width: parent ? parent.width : 0
    implicitHeight: root.pageState.derivedMetrics.length > 0 ? metricsCard.implicitHeight : 0
    visible: root.pageState.derivedMetrics.length > 0

    SectionCard {
        id: metricsCard

        width: root.width
        theme: root.theme
        title: qsTr("Derived Metrics")
        subtitle: qsTr("Quick engineering feedback based on the current form values.")

        Flow {
            width: parent.width
            spacing: 10

            Repeater {
                model: root.pageState.derivedMetrics

                delegate: Rectangle {
                    id: metricCard

                    required property var modelData

                    width: Math.max(150, (parent.width - 10) / (root.pageState.derivedMetrics.length > 2 ? 3 : 2))
                    height: metricColumn.implicitHeight + 18
                    radius: root.theme.radiusSmall
                    color: root.theme.tint(root.theme.resolveAccentColor(modelData.accent), root.theme.darkMode ? 0.14 : 0.08)
                    border.width: 1
                    border.color: root.theme.tint(root.theme.resolveAccentColor(modelData.accent), root.theme.darkMode ? 0.46 : 0.24)

                    Column {
                        id: metricColumn

                        anchors.fill: parent
                        anchors.margins: 9
                        spacing: 4

                        Text {
                            width: parent.width
                            text: metricCard.modelData.label
                            color: root.theme.textSecondary
                            font.pixelSize: 10
                            font.bold: true
                            font.family: root.theme.bodyFontFamily
                            wrapMode: Text.Wrap
                        }

                        Text {
                            width: parent.width
                            text: metricCard.modelData.value
                            color: root.theme.textPrimary
                            font.pixelSize: 13
                            font.bold: true
                            font.family: root.theme.monoFontFamily
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
        }
    }
}
