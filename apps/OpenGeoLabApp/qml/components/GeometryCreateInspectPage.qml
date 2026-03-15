pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import "../theme"

Item {
    id: root

    required property AppTheme theme
    required property var pageState

    readonly property string stateText: {
        const currentState = root.pageState.appController.operationState;
        if (root.pageState.requestPending || currentState === "running") {
            return qsTr("Running");
        }
        if (currentState === "success") {
            return qsTr("Succeeded");
        }
        if (currentState === "error") {
            return qsTr("Failed");
        }
        return qsTr("Idle");
    }
    readonly property color stateTint: {
        if (root.pageState.requestPending || root.pageState.appController.operationState === "running") {
            return root.theme.accentA;
        }
        if (root.pageState.appController.operationState === "success") {
            return root.theme.accentB;
        }
        if (root.pageState.appController.operationState === "error") {
            return root.theme.accentD;
        }
        return root.theme.accentE;
    }
    readonly property string activityMessage: {
        if (root.pageState.requestPending) {
            return qsTr("Geometry request is queued for execution.");
        }
        if (root.pageState.appController.operationMessage.length > 0) {
            return root.pageState.appController.operationMessage;
        }
        if (root.pageState.appController.lastSummary.length > 0) {
            return root.pageState.appController.lastSummary;
        }
        return qsTr("No geometry request has been executed yet.");
    }

    width: parent ? parent.width : 0
    implicitHeight: contentColumn.implicitHeight

    ColumnLayout {
        id: contentColumn

        width: root.width
        spacing: root.theme.gap

        SectionCard {
            Layout.fillWidth: true
            theme: root.theme
            title: qsTr("Operation Snapshot")
            subtitle: qsTr("Review the latest request payload and execution feedback before iterating.")

            Column {
                width: parent.width
                spacing: 10

                Flow {
                    width: parent.width
                    spacing: 8

                    StatChip {
                        theme: root.theme
                        text: root.stateText
                        tintColor: root.stateTint
                    }

                    StatChip {
                        visible: root.pageState.appController.lastModule.length > 0
                        theme: root.theme
                        text: qsTr("Module %1").arg(root.pageState.appController.lastModule)
                        tintColor: root.theme.accentE
                    }

                    StatChip {
                        theme: root.theme
                        text: qsTr("Recorded %1").arg(root.pageState.appController.recordedCommandCount)
                        tintColor: root.theme.accentB
                    }
                }

                Text {
                    width: parent.width
                    text: root.activityMessage
                    wrapMode: Text.WordWrap
                    color: root.theme.textSecondary
                    font.pixelSize: 12
                    font.family: root.theme.bodyFontFamily
                }

                Text {
                    width: parent.width
                    text: qsTr("Use the viewport Activity badge for the detailed activity timeline and progress feedback.")
                    wrapMode: Text.WordWrap
                    color: root.theme.textTertiary
                    font.pixelSize: 11
                    font.family: root.theme.bodyFontFamily
                }
            }
        }

        CodePanel {
            Layout.fillWidth: true
            theme: root.theme
            title: qsTr("Request Preview")
            bodyText: root.pageState.requestPreviewText()
            minimumBodyHeight: 118
            maximumBodyHeight: 220
        }

        CodePanel {
            Layout.fillWidth: true
            theme: root.theme
            title: qsTr("Last Geometry Response")
            bodyText: root.pageState.responsePreviewText()
            minimumBodyHeight: 140
            maximumBodyHeight: 260
        }

        CodePanel {
            Layout.fillWidth: true
            theme: root.theme
            title: qsTr("Recorded Commands")
            bodyText: root.pageState.appController.recordedCommands
            minimumBodyHeight: 118
            maximumBodyHeight: 220
        }
    }
}
