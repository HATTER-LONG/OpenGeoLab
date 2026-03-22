pragma ComponentBehavior: Bound

import QtQuick

import ".."
import "../theme"
import "../components" as Components

Rectangle {
    id: panel

    required property AppTheme theme
    required property bool darkMode
    required property bool menuOpen
    required property var actionHandler

    MenuConfig {
        id: menuConfig
    }

    visible: panel.menuOpen
    z: 10
    x: 0
    y: 34
    width: 248
    radius: 14
    color: panel.theme.surface
    border.width: 1
    border.color: panel.theme.borderSubtle
    opacity: panel.menuOpen ? 1 : 0
    scale: panel.menuOpen ? 1 : 0.96
    implicitHeight: menuColumn.implicitHeight + 24

    Behavior on opacity {
        NumberAnimation {
            duration: 140
        }
    }

    Behavior on scale {
        NumberAnimation {
            duration: 140
            easing.type: Easing.OutCubic
        }
    }

    Column {
        id: menuColumn

        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Repeater {
            model: menuConfig.sections

            delegate: Column {
                id: sectionDelegate

                required property int index
                required property var modelData

                readonly property string sectionAccent: modelData.accent

                width: menuColumn.width
                spacing: 10

                Text {
                    text: sectionDelegate.modelData.title
                    color: panel.theme.textSecondary
                    font.pixelSize: 12
                    font.bold: true
                }

                Rectangle {
                    width: parent.width
                    radius: 14
                    color: sectionDelegate.index === 0 ? panel.theme.panel.menuBg : panel.theme.panel.menuRecorderBg
                    border.width: 1
                    border.color: panel.theme.panel.menuBorder
                    implicitHeight: actionColumn.implicitHeight + 20

                    Column {
                        id: actionColumn

                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        Repeater {
                            model: sectionDelegate.modelData.actions

                            delegate: Components.ActionButton {
                                required property var modelData

                                readonly property string effectiveAccent: modelData.accent ?? sectionDelegate.sectionAccent
                                readonly property string effectiveAlpha: modelData.alphaScale ?? "normal"

                                theme: panel.theme
                                width: actionColumn.width
                                leftAligned: true
                                actionKey: modelData.key
                                actionHandler: panel.actionHandler
                                colorSet: panel.theme.actionButtonColors(effectiveAccent, effectiveAlpha)
                                hoverBorderOverride: modelData.hoverAccent
                                    ? panel.theme.accentHoverBorder(modelData.hoverAccent) : "transparent"
                                buttonText: {
                                    if (modelData.key === "toggleTheme")
                                        return panel.darkMode ? qsTr("Switch to Light") : qsTr("Switch to Dark");
                                    if (modelData.key === "switchLanguage")
                                        return TranslationManager.currentLanguage === "zh_CN"
                                            ? qsTr("Switch to English") : qsTr("Switch to Chinese");
                                    return modelData.title;
                                }
                                iconKind: {
                                    if (modelData.key === "toggleTheme")
                                        return panel.darkMode ? "lightTheme" : "darkTheme";
                                    return modelData.icon;
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: panel.theme.panel.separator
                    visible: sectionDelegate.index < menuConfig.sections.length - 1
                }
            }
        }
    }
}
