pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import "GeometryCreatePageLogic.js" as GeometryCreateLogic

FeaturePageBase {
    id: page

    required property var appController
    property var actionDefinition: null
    property string validationMessage: ""
    property string invalidFieldKey: ""
    property var formValues: ({})
    property string axisValue: "Z"
    property bool requestPending: false
    property int queuedExecutionId: 0
    property int pendingRequestId: -1
    readonly property var requestSpec: page.actionDefinition && page.actionDefinition.requestSpec ? page.actionDefinition.requestSpec : null
    readonly property string requestSource: "qml-ui"
    readonly property string requestModuleName: requestSpec ? requestSpec.module : ""
    readonly property string requestActionName: requestSpec ? requestSpec.action : ""
    readonly property var positionFields: requestSpec && requestSpec.positionFields ? requestSpec.positionFields : []
    readonly property var dimensionFields: requestSpec && requestSpec.dimensionFields ? requestSpec.dimensionFields : []
    readonly property var axisOptions: requestSpec && requestSpec.axisOptions ? requestSpec.axisOptions : []
    readonly property bool supportsAxis: axisOptions.length > 0
    readonly property string advisoryMessage: GeometryCreateLogic.advisoryMessage(page)
    readonly property var derivedMetrics: GeometryCreateLogic.derivedMetrics(page)

    closeOnExecute: false
    maxPanelWidth: 640
    maxPanelHeight: 820
    minPanelHeight: 520
    statusBadgeText: qsTr("Geometry Create")
    executeButtonText: requestPending ? qsTr("Creating...") : qsTr("Create")
    pageTitle: page.actionDefinition ? page.actionDefinition.pageTitle : qsTr("Geometry Create")
    sectionTitle: page.actionDefinition ? page.actionDefinition.sectionTitle : qsTr("Workbench")
    summaryText: page.actionDefinition ? page.actionDefinition.summary : ""
    iconKind: page.actionDefinition ? page.actionDefinition.icon : "box"
    accentName: page.actionDefinition ? page.actionDefinition.accent : "accentA"

    function refreshAction(nextActionDefinition) {
        page.actionDefinition = nextActionDefinition;
    }

    function resolveAccentColorByName(name) {
        if (name === "accentB") {
            return page.theme.accentB;
        }
        if (name === "accentC") {
            return page.theme.accentC;
        }
        if (name === "accentD") {
            return page.theme.accentD;
        }
        if (name === "accentE") {
            return page.theme.accentE;
        }
        return page.theme.accentA;
    }

    function fieldAccentColor(field) {
        return resolveAccentColorByName(field && field.accent ? field.accent : page.accentName);
    }

    function presentAction(nextActionDefinition) {
        page.actionDefinition = nextActionDefinition;
        queuedExecutionId += 1;
        requestPending = false;
        pendingRequestId = -1;
        resetForm();
        page.validationMessage = "";
        page.invalidFieldKey = "";
        present();
    }

    function resetForm() {
        GeometryCreateLogic.resetForm(page);
    }

    function fieldValue(fieldKey) {
        return GeometryCreateLogic.fieldValue(page, fieldKey);
    }

    function setFieldValue(fieldKey, nextValue) {
        GeometryCreateLogic.setFieldValue(page, fieldKey, nextValue);
    }

    function setEditedFieldValue(fieldKey, nextValue) {
        setFieldValue(fieldKey, nextValue);
        if (page.invalidFieldKey === fieldKey) {
            page.invalidFieldKey = "";
            page.validationMessage = "";
        }
    }

    function buildValidatedRequest() {
        return GeometryCreateLogic.buildValidatedRequest(page);
    }

    function executeGeometryCreate() {
        if (requestPending) {
            return;
        }

        const result = buildValidatedRequest();
        if (!result.success) {
            page.invalidFieldKey = result.fieldKey;
            page.validationMessage = result.message;
            return;
        }

        const requestJson = JSON.stringify(result.request);
        const executionId = queuedExecutionId + 1;
        queuedExecutionId = executionId;
        page.requestPending = true;
        page.invalidFieldKey = "";
        page.validationMessage = "";

        Qt.callLater(function () {
            if (page.queuedExecutionId !== executionId) {
                return;
            }

            const requestId = appController.submitServiceRequest(requestJson);
            if (page.queuedExecutionId !== executionId || requestId < 0) {
                page.requestPending = false;
                page.validationMessage = appController.lastSummary && appController.lastSummary.length > 0
                    ? appController.lastSummary
                    : qsTr("Geometry create request failed.");
                return;
            }
            page.pendingRequestId = requestId;
        });
    }

    onExecuteRequested: executeGeometryCreate()
    onCancelRequested: {
        page.validationMessage = "";
        page.invalidFieldKey = "";
        page.requestPending = false;
        page.pendingRequestId = -1;
        page.queuedExecutionId += 1;
    }

    Connections {
        target: appController

        function onServiceRequestFinished(requestId, success) {
            if (requestId !== page.pendingRequestId) {
                return;
            }

            page.requestPending = false;
            page.pendingRequestId = -1;
            if (success) {
                return;
            }

            page.validationMessage = appController.lastSummary && appController.lastSummary.length > 0
                ? appController.lastSummary
                : qsTr("Geometry create request failed.");
        }
    }

    StatusMessageBanner {
        Layout.fillWidth: true
        theme: page.theme
        message: page.validationMessage
        badgeText: qsTr("Validation")
        accentColor: page.theme.accentD
    }

    StatusMessageBanner {
        Layout.fillWidth: true
        theme: page.theme
        message: page.advisoryMessage
        badgeText: qsTr("Advisory")
        accentColor: page.theme.accentC
    }

    SectionCard {
        Layout.fillWidth: true
        theme: page.theme
        title: qsTr("Parameters")
        subtitle: page.summaryText

        Flow {
            width: parent.width
            spacing: 8

            StatChip {
                theme: page.theme
                text: page.requestModuleName
                tintColor: page.theme.accentA
            }

            StatChip {
                theme: page.theme
                text: page.requestActionName
                tintColor: page.theme.accentB
            }
        }

        ParameterInputField {
            width: parent.width
            theme: page.theme
            label: qsTr("Model Name")
            value: page.fieldValue("modelName")
            placeholderText: page.requestSpec ? page.requestSpec.defaultName : ""
            accentColor: page.accentColor
            showAccentMarker: true
            onValueEdited: function (value) {
                page.setEditedFieldValue("modelName", value);
            }
        }
    }

    SectionCard {
        Layout.fillWidth: true
        theme: page.theme
        title: page.requestSpec ? page.requestSpec.positionTitle : qsTr("Placement")
        subtitle: qsTr("Set the placement coordinates used by the current geometry action.")

        GridLayout {
            width: parent.width
            columns: Math.max(1, Math.min(3, page.positionFields.length))
            rowSpacing: 10
            columnSpacing: 10

            Repeater {
                model: page.positionFields

                delegate: ParameterInputField {
                    required property var modelData

                    Layout.fillWidth: true
                    theme: page.theme
                    label: modelData.label
                    value: page.fieldValue(modelData.key)
                    unit: modelData.unit
                    numeric: true
                    invalid: page.invalidFieldKey === modelData.key
                    accentColor: page.fieldAccentColor(modelData)
                    showAccentMarker: true
                    placeholderText: modelData.defaultValue
                    onValueEdited: function (value) {
                        page.setEditedFieldValue(modelData.key, value);
                    }
                }
            }
        }
    }

    SectionCard {
        Layout.fillWidth: true
        theme: page.theme
        title: page.requestSpec ? page.requestSpec.dimensionTitle : qsTr("Dimensions")
        subtitle: qsTr("Edit the driving dimensions that will be sent to the geometry service.")

        GridLayout {
            width: parent.width
            columns: Math.max(1, Math.min(3, page.dimensionFields.length))
            rowSpacing: 10
            columnSpacing: 10

            Repeater {
                model: page.dimensionFields

                delegate: ParameterInputField {
                    required property var modelData

                    Layout.fillWidth: true
                    theme: page.theme
                    label: modelData.label
                    value: page.fieldValue(modelData.key)
                    unit: modelData.unit
                    numeric: true
                    invalid: page.invalidFieldKey === modelData.key
                    accentColor: page.fieldAccentColor(modelData)
                    showAccentMarker: true
                    placeholderText: modelData.defaultValue
                    onValueEdited: function (value) {
                        page.setEditedFieldValue(modelData.key, value);
                    }
                }
            }
        }
    }

    SectionCard {
        Layout.fillWidth: true
        visible: page.supportsAxis
        theme: page.theme
        title: qsTr("Axis")
        subtitle: qsTr("Choose the construction axis forwarded to the geometry action.")

        Flow {
            width: parent.width
            spacing: 10

            Repeater {
                model: page.axisOptions

                delegate: AxisOptionChip {
                    required property var modelData

                    theme: page.theme
                    label: modelData
                    accentColor: page.resolveAccentColorByName(modelData === "X" ? "accentD" : (modelData === "Y" ? "accentB" : "accentE"))
                    selected: page.axisValue === modelData
                    onClicked: page.axisValue = modelData
                }
            }
        }
    }

    SectionCard {
        Layout.fillWidth: true
        visible: page.derivedMetrics.length > 0
        theme: page.theme
        title: qsTr("Derived Metrics")
        subtitle: qsTr("Quick engineering feedback based on the current form values.")

        Flow {
            width: parent.width
            spacing: 10

            Repeater {
                model: page.derivedMetrics

                delegate: Rectangle {
                    required property var modelData

                    width: Math.max(150, (parent.width - 10) / (page.derivedMetrics.length > 2 ? 3 : 2))
                    height: metricColumn.implicitHeight + 18
                    radius: page.theme.radiusSmall
                    color: page.theme.tint(page.resolveAccentColorByName(modelData.accent), page.theme.darkMode ? 0.14 : 0.08)
                    border.width: 1
                    border.color: page.theme.tint(page.resolveAccentColorByName(modelData.accent), page.theme.darkMode ? 0.46 : 0.24)

                    Column {
                        id: metricColumn

                        anchors.fill: parent
                        anchors.margins: 9
                        spacing: 4

                        Text {
                            width: parent.width
                            text: modelData.label
                            color: page.theme.textSecondary
                            font.pixelSize: 10
                            font.bold: true
                            font.family: page.theme.bodyFontFamily
                            wrapMode: Text.Wrap
                        }

                        Text {
                            width: parent.width
                            text: modelData.value
                            color: page.theme.textPrimary
                            font.pixelSize: 13
                            font.bold: true
                            font.family: page.theme.monoFontFamily
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
        }
    }
}
