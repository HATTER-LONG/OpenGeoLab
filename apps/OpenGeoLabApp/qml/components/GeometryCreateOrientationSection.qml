pragma ComponentBehavior: Bound

import QtQuick

import "../theme"

Item {
    id: root

    required property AppTheme theme
    required property var pageState
    property string title: qsTr("Orientation")
    property string subtitle: qsTr("Choose the creation axis for primitives that support directional placement.")

    width: parent ? parent.width : 0
    implicitHeight: root.pageState.supportsAxis ? orientationCard.implicitHeight : 0
    visible: root.pageState.supportsAxis

    SectionCard {
        id: orientationCard

        width: root.width
        theme: root.theme
        title: root.title
        subtitle: root.subtitle

        Column {
            width: parent.width
            spacing: 10

            Flow {
                width: parent.width
                spacing: 8

                Repeater {
                    model: root.pageState.axisOptions

                    delegate: AxisOptionChip {
                        required property var modelData

                        theme: root.theme
                        label: modelData
                        accentColor: root.pageState.axisAccentColor(modelData)
                        selected: root.pageState.axisValue === modelData
                        onClicked: root.pageState.axisValue = modelData
                    }
                }
            }

            StatChip {
                theme: root.theme
                text: qsTr("Current axis %1").arg(root.pageState.axisValue)
                tintColor: root.pageState.axisAccentColor(root.pageState.axisValue)
            }
        }
    }
}
