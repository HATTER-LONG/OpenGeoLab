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

    // Optional field schema; each entry: { key, label, type: 'string'|'number'|'bool', placeholder, default }
    // If null/empty, uses defaultFields.
    property var fields: null

    // Collected parameters (kept in JS object to avoid relying on Repeater item access).
    property var params: ({})

    readonly property var defaultFields: ([
            {
                key: "paramA",
                label: qsTr("paramA"),
                type: "string",
                placeholder: qsTr("paramA")
            },
            {
                key: "paramB",
                label: qsTr("paramB"),
                type: "string",
                placeholder: qsTr("paramB")
            }
        ])

    readonly property var _effectiveFields: {
        if (dlg.fields && dlg.fields.length && dlg.fields.length > 0)
            return dlg.fields;
        return dlg.defaultFields;
    }

    function _initialValueFor(field): var {
        const key = field.key;
        if (dlg.initialParams && key && dlg.initialParams[key] !== undefined)
            return dlg.initialParams[key];
        return field.default;
    }

    function _setParam(key, value): void {
        if (!key)
            return;
        const next = Object.assign({}, dlg.params);
        next[key] = value;
        dlg.params = next;
    }

    function _resetParams(): void {
        const next = {};
        const fs = dlg._effectiveFields || [];
        for (let i = 0; i < fs.length; i++) {
            const f = fs[i];
            if (!f || !f.key)
                continue;
            next[f.key] = dlg._initialValueFor(f);
        }
        dlg.params = next;
    }

    onFieldsChanged: dlg._resetParams()
    onInitialParamsChanged: dlg._resetParams()
    Component.onCompleted: dlg._resetParams()

    signal closeRequested

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

                Repeater {
                    id: fieldsRepeater
                    model: dlg._effectiveFields

                    RowLayout {
                        id: row
                        required property var modelData

                        Layout.fillWidth: true
                        spacing: 8

                        readonly property string fieldKey: modelData.key || ""
                        readonly property string fieldLabel: modelData.label || fieldKey
                        readonly property string fieldType: modelData.type || "string"
                        readonly property string fieldPlaceholder: modelData.placeholder || ""
                        readonly property var fieldInitial: dlg._initialValueFor(modelData)

                        Label {
                            Layout.preferredWidth: 110
                            text: row.fieldLabel
                            color: Theme.textPrimaryColor
                            elide: Text.ElideRight
                        }

                        TextField {
                            id: textEditor
                            Layout.fillWidth: true
                            visible: row.fieldType !== "bool"
                            enabled: !OGL.BackendService.busy
                            placeholderText: row.fieldPlaceholder

                            Component.onCompleted: {
                                if (row.fieldInitial === undefined || row.fieldInitial === null)
                                    text = "";
                                else
                                    text = String(row.fieldInitial);
                                // Initialize params
                                if (row.fieldType === "number") {
                                    const n = Number(text);
                                    dlg._setParam(row.fieldKey, isNaN(n) ? undefined : n);
                                } else {
                                    dlg._setParam(row.fieldKey, text);
                                }
                            }

                            onTextChanged: {
                                if (row.fieldType === "number") {
                                    const n = Number(text);
                                    dlg._setParam(row.fieldKey, isNaN(n) ? undefined : n);
                                } else {
                                    dlg._setParam(row.fieldKey, text);
                                }
                            }
                        }

                        CheckBox {
                            id: boolEditor
                            Layout.fillWidth: true
                            visible: row.fieldType === "bool"
                            enabled: !OGL.BackendService.busy
                            text: row.fieldPlaceholder

                            Component.onCompleted: {
                                checked = Boolean(row.fieldInitial);
                                dlg._setParam(row.fieldKey, checked);
                            }

                            onCheckedChanged: dlg._setParam(row.fieldKey, checked)
                        }
                    }
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
                    OGL.BackendService.request(dlg.actionId, dlg.params || ({}));
                }
            }

            Button {
                text: qsTr("Cancel")
                enabled: !OGL.BackendService.busy
                onClicked: dlg.closeRequested()
            }
        }

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
            if (failedActionId === dlg.actionId) {
                // keep open
            }
        }
    }
}
