pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

FeaturePageBase {
    id: page

    property var actionDefinition: null
    readonly property var actionFocusPoints: page.actionDefinition && page.actionDefinition.focusPoints ? page.actionDefinition.focusPoints : []
    statusBadgeText: qsTr("Action")

    pageTitle: page.actionDefinition ? page.actionDefinition.pageTitle : qsTr("Feature Page")
    sectionTitle: page.actionDefinition ? page.actionDefinition.sectionTitle : qsTr("Workbench")
    summaryText: page.actionDefinition ? page.actionDefinition.summary : qsTr("This non-modal in-app panel reserves the future workflow surface for the selected action.")
    iconKind: page.actionDefinition ? page.actionDefinition.icon : "menu"
    accentName: page.actionDefinition ? page.actionDefinition.accent : "accentA"

    function presentAction(nextActionDefinition) {
        page.actionDefinition = nextActionDefinition;
        present();
    }

    function refreshAction(nextActionDefinition) {
        page.actionDefinition = nextActionDefinition;
    }

    Rectangle {
        Layout.fillWidth: true
        radius: page.theme.radiusMedium
        color: page.theme.tint(page.theme.surfaceMuted, page.theme.darkMode ? 0.64 : 0.9)
        border.width: 1
        border.color: page.theme.tint(page.theme.borderSubtle, 0.82)
        implicitHeight: overviewColumn.implicitHeight + 20

        Column {
            id: overviewColumn

            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            Text {
                width: parent.width
                text: qsTr("Action Scope")
                color: page.theme.textPrimary
                font.pixelSize: 14
                font.bold: true
                font.family: page.theme.bodyFontFamily
            }

            Text {
                width: parent.width
                text: page.actionDefinition ? qsTr("Action key: %1").arg(page.actionDefinition.key) : qsTr("Action key: -")
                color: page.theme.textSecondary
                font.pixelSize: 11
                font.family: page.theme.monoFontFamily
                wrapMode: Text.Wrap
            }

            Text {
                width: parent.width
                text: page.summaryText
                color: page.theme.textSecondary
                font.pixelSize: 12
                font.family: page.theme.bodyFontFamily
                wrapMode: Text.Wrap
            }

            Text {
                width: parent.width
                text: qsTr("This is the shared non-modal action page shell for the selected action. The final workflow UI can later replace or extend this content without changing the action entry points again.")
                color: page.theme.textSecondary
                font.pixelSize: 12
                font.family: page.theme.bodyFontFamily
                wrapMode: Text.Wrap
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        radius: page.theme.radiusMedium
        color: page.theme.tint(page.theme.surfaceMuted, page.theme.darkMode ? 0.48 : 0.82)
        border.width: 1
        border.color: page.theme.tint(page.theme.borderSubtle, 0.76)
        implicitHeight: milestoneColumn.implicitHeight + 20

        Column {
            id: milestoneColumn

            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            Text {
                width: parent.width
                text: qsTr("Planned Integration")
                color: page.theme.textPrimary
                font.pixelSize: 14
                font.bold: true
                font.family: page.theme.bodyFontFamily
            }

            Text {
                width: parent.width
                text: page.actionDefinition ? page.actionDefinition.nextMilestone : qsTr("The next implementation milestone has not been configured yet.")
                color: page.theme.textSecondary
                font.pixelSize: 12
                font.family: page.theme.bodyFontFamily
                wrapMode: Text.Wrap
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        radius: page.theme.radiusMedium
        color: page.theme.tint(page.theme.surfaceMuted, page.theme.darkMode ? 0.42 : 0.78)
        border.width: 1
        border.color: page.theme.tint(page.theme.borderSubtle, 0.72)
        implicitHeight: focusColumn.implicitHeight + 20

        Column {
            id: focusColumn

            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            Text {
                width: parent.width
                text: qsTr("Expected Focus")
                color: page.theme.textPrimary
                font.pixelSize: 14
                font.bold: true
                font.family: page.theme.bodyFontFamily
            }

            Flow {
                width: parent.width
                spacing: 8

                Repeater {
                    model: page.actionFocusPoints

                    delegate: Rectangle {
                        required property var modelData

                        radius: 12
                        color: page.theme.tint(page.accentColor, page.theme.darkMode ? 0.18 : 0.1)
                        border.width: 1
                        border.color: page.theme.tint(page.accentColor, page.theme.darkMode ? 0.44 : 0.22)
                        implicitWidth: chipLabel.implicitWidth + 18
                        implicitHeight: chipLabel.implicitHeight + 10

                        Text {
                            id: chipLabel

                            anchors.centerIn: parent
                            text: parent.modelData
                            color: page.theme.textPrimary
                            font.pixelSize: 11
                            font.bold: true
                            font.family: page.theme.bodyFontFamily
                        }
                    }
                }
            }
        }
    }
}
