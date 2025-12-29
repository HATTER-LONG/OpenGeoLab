pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL

Item {
    id: dlg

    property string actionId: ""
    property string title: ""
    property var initialParams: ({})

    signal closeRequested

    // This dialog is intentionally generic; each action can later have its own dialog.
    implicitWidth: 420
    implicitHeight: layout.implicitHeight + 24

    Rectangle {
        anchors.fill: parent
        color: Theme.surfaceColor
        border.width: 1
        border.color: Theme.borderColor
        radius: 6
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: dlg.title.length > 0 ? dlg.title : dlg.actionId
                color: Theme.textPrimaryColor
                font.bold: true
            }

            Button {
                text: qsTr("X")
                onClicked: dlg.closeRequested()
            }
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("ActionId: %1").arg(dlg.actionId)
            color: Theme.textSecondaryColor
            wrapMode: Text.Wrap
        }

        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Parameters")

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                TextField {
                    id: paramA
                    Layout.fillWidth: true
                    placeholderText: qsTr("paramA")
                    text: (dlg.initialParams && dlg.initialParams.paramA !== undefined) ? String(dlg.initialParams.paramA) : ""
                    enabled: !OGL.BackendService.busy
                }

                TextField {
                    id: paramB
                    Layout.fillWidth: true
                    placeholderText: qsTr("paramB")
                    text: (dlg.initialParams && dlg.initialParams.paramB !== undefined) ? String(dlg.initialParams.paramB) : ""
                    enabled: !OGL.BackendService.busy
                }
            }
        }

        Label {
            Layout.fillWidth: true
            text: OGL.BackendService.lastError
            visible: OGL.BackendService.lastError.length > 0
            color: Theme.textSecondaryColor
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: qsTr("Run")
                enabled: !OGL.BackendService.busy && dlg.actionId.length > 0
                onClicked: {
                    const params = {
                        paramA: paramA.text,
                        paramB: paramB.text
                    };
                    OGL.BackendService.request(dlg.actionId, params);
                }
            }

            Button {
                text: qsTr("Cancel")
                enabled: !OGL.BackendService.busy
                onClicked: dlg.closeRequested()
            }
        }

        // Keep dialog responsive; progress is shown by the global overlay as well.
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: OGL.BackendService.busy

            BusyIndicator {
                running: OGL.BackendService.busy
                visible: OGL.BackendService.busy
            }

            Label {
                Layout.fillWidth: true
                text: OGL.BackendService.message
                color: Theme.textSecondaryColor
                elide: Text.ElideRight
            }
        }
    }

    Connections {
        target: OGL.BackendService

        function onOperationFinished(doneActionId, _result) {
            if (doneActionId === dlg.actionId)
                dlg.closeRequested();
        }

        function onOperationFailed(failedActionId, _error) {
            // Keep dialog open so user can adjust params.
            if (failedActionId === dlg.actionId) {
                // no-op
            }
        }
    }
}
