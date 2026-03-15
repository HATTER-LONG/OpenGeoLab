pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import "." as HeaderSections
import "../theme"
import "../components" as Components

Rectangle {
    id: header

    required property AppTheme theme
    required property bool darkMode
    required property bool menuOpen
    required property int selectedTab
    property string currentLanguage: "en_US"
    property int recordedCommandCount: 0
    property var ribbonTabs: []
    property var ribbonGroups: []
    property int ribbonButtonWidth: 68
    signal toggleMenu
    signal requestThemeToggle
    signal requestLanguageToggle
    signal selectTab(int index)
    signal triggerAction(string actionKey)

    radius: theme.radiusMedium
    z: 20
    // color: theme.surface
    color: "transparent"
    border.width: 0
    border.color: theme.borderSubtle
    implicitHeight: 136

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 0
        spacing: 4

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            radius: 12
            color: header.theme.tint(header.theme.surface, header.theme.darkMode ? 0.7 : 1.0)
            border.width: 1
            border.color: header.theme.tint(header.theme.borderSubtle, 0.78)

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.rightMargin: 10
                spacing: 6

                Rectangle {
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 22
                    radius: 8
                    color: menuMouseArea.pressed ? header.theme.tint(header.theme.surfaceStrong, 0.94) : (menuMouseArea.containsMouse ? header.theme.tint(header.theme.surfaceStrong, 0.82) : "transparent")
                    border.width: menuMouseArea.containsMouse ? 1 : 0
                    border.color: header.theme.tint(header.theme.borderSubtle, 0.74)

                    Components.AppIcon {
                        anchors.centerIn: parent
                        theme: header.theme
                        iconKind: "menu"
                        primaryColor: header.theme.accentA
                        width: 15
                        height: 15
                    }

                    MouseArea {
                        id: menuMouseArea

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: header.toggleMenu()
                    }
                }

                Repeater {
                    model: header.ribbonTabs

                    delegate: Rectangle {
                        id: tabButton

                        required property int index
                        required property var modelData

                        readonly property bool active: tabButton.index === header.selectedTab

                        Layout.preferredWidth: Math.max(tabLabel.implicitWidth + 18, 62)
                        Layout.preferredHeight: 22
                        radius: 8
                        color: tabButton.active ? header.theme.tint(header.theme.accentA, header.theme.darkMode ? 0.2 : 0.12) : (tabArea.containsMouse ? header.theme.tint(header.theme.surfaceStrong, header.theme.darkMode ? 0.54 : 0.74) : "transparent")
                        border.width: active ? 1 : 0
                        border.color: header.theme.tint(header.theme.accentA, header.theme.darkMode ? 0.42 : 0.24)

                        Text {
                            id: tabLabel

                            anchors.centerIn: parent
                            text: tabButton.modelData
                            color: tabButton.active ? header.theme.textPrimary : header.theme.textSecondary
                            font.pixelSize: 11
                            font.bold: tabButton.active
                            font.family: header.theme.bodyFontFamily
                        }

                        MouseArea {
                            id: tabArea

                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: header.selectTab(tabButton.index)
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }

        // Rectangle {
        //     Layout.fillWidth: true
        //     Layout.preferredHeight: 1
        //     color: header.theme.tint(header.theme.borderSubtle, 0.62)
        // }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 14
            color: header.theme.tint(header.theme.surface, header.theme.darkMode ? 0.7 : 1.0)
            border.width: 1
            border.color: header.theme.tint(header.theme.borderSubtle, 0.7)

            Flickable {
                anchors.fill: parent
                anchors.margins: 6
                contentWidth: groupRow.implicitWidth
                contentHeight: height
                clip: true
                boundsBehavior: Flickable.StopAtBounds

                Row {
                    id: groupRow

                    height: parent.height
                    spacing: 0

                    Repeater {
                        model: header.ribbonGroups

                        delegate: Item {
                            required property int index
                            required property var modelData

                            width: ribbonGroup.width
                            height: parent ? parent.height : ribbonGroup.implicitHeight

                            HeaderSections.HeaderRibbonGroup {
                                id: ribbonGroup

                                anchors.fill: parent
                                theme: header.theme
                                groupData: parent.modelData
                                groupIndex: parent.index
                                groupCount: header.ribbonGroups.length
                                buttonSize: header.ribbonButtonWidth
                                onTriggerAction: function (actionKey) {
                                    header.triggerAction(actionKey);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    HeaderSections.HeaderMenuPanel {
        theme: header.theme
        darkMode: header.darkMode
        menuOpen: header.menuOpen
        currentLanguage: header.currentLanguage
        onRequestThemeToggle: header.requestThemeToggle()
        onRequestLanguageToggle: header.requestLanguageToggle()
        onTriggerAction: function (actionKey) {
            header.triggerAction(actionKey);
        }
    }
}
