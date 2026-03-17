pragma ComponentBehavior: Bound

import QtQml

import "GeometryCreatePageLogic.js" as GeometryCreateLogic

QtObject {
    id: root

    required property var actionDefinition
    property string requestSource: "qml-ui"
    property var formValues: ({})
    property string axisValue: "Z"
    property string validationMessage: ""
    property string invalidFieldKey: ""
    property var derivedMetrics: []
    property string requestJson: ""
    readonly property var requestSpec: actionDefinition && actionDefinition.requestSpec ? actionDefinition.requestSpec : null
    readonly property string accentName: actionDefinition && actionDefinition.accent ? actionDefinition.accent : "accentA"
    readonly property string requestModuleName: requestSpec ? requestSpec.module : ""
    readonly property string requestActionName: requestSpec ? requestSpec.action : ""
    readonly property var positionFields: requestSpec && requestSpec.positionFields ? requestSpec.positionFields : []
    readonly property var placementFields: positionFields
    readonly property var dimensionFields: requestSpec && requestSpec.dimensionFields ? requestSpec.dimensionFields : []
    readonly property var axisOptions: requestSpec && requestSpec.axisOptions ? requestSpec.axisOptions : []
    readonly property bool supportsAxis: axisOptions.length > 0
    readonly property string positionTitle: requestSpec && requestSpec.positionTitle ? requestSpec.positionTitle : qsTr("Placement")
    readonly property string dimensionTitle: requestSpec && requestSpec.dimensionTitle ? requestSpec.dimensionTitle : qsTr("Dimensions")
    property string translationTrigger: qsTr("Geometry Create")
    readonly property string advisoryMessage: {
        translationTrigger;
        return GeometryCreateLogic.advisoryMessage(requestSpec, formValues);
    }

    function refreshPresentationState() {
        refreshDerivedState();
        if (invalidFieldKey.length > 0 && validationMessage.length > 0) {
            const validationResult = buildValidatedRequest();
            if (!validationResult.success && validationResult.fieldKey === invalidFieldKey) {
                validationMessage = validationResult.message;
            }
        }
    }

    function refreshDerivedState() {
        derivedMetrics = GeometryCreateLogic.derivedMetrics(requestSpec, formValues);
        requestJson = GeometryCreateLogic.requestJson({
            "requestSpec": requestSpec,
            "requestSource": requestSource,
            "positionFields": positionFields,
            "dimensionFields": dimensionFields,
            "supportsAxis": supportsAxis,
            "axisValue": axisValue,
            "formValues": formValues
        });
    }

    function reset() {
        formValues = GeometryCreateLogic.createInitialFormValues(requestSpec, positionFields, dimensionFields);
        axisValue = GeometryCreateLogic.defaultAxis(requestSpec);
        validationMessage = "";
        invalidFieldKey = "";
        refreshDerivedState();
    }

    function clearValidation() {
        validationMessage = "";
        invalidFieldKey = "";
    }

    function fieldValue(fieldKey) {
        return GeometryCreateLogic.fieldValue(formValues, fieldKey);
    }

    function setFieldValue(fieldKey, nextValue) {
        formValues = GeometryCreateLogic.withFieldValue(formValues, fieldKey, nextValue);
        refreshDerivedState();
    }

    function setEditedFieldValue(fieldKey, nextValue) {
        setFieldValue(fieldKey, nextValue);
        if (invalidFieldKey === fieldKey) {
            clearValidation();
        }
    }

    function buildValidatedRequest() {
        return GeometryCreateLogic.buildValidatedRequest({
            "requestSpec": requestSpec,
            "requestSource": requestSource,
            "positionFields": positionFields,
            "dimensionFields": dimensionFields,
            "supportsAxis": supportsAxis,
            "axisValue": axisValue,
            "formValues": formValues
        });
    }

    function axisAccentName(axis) {
        if (axis === "X") {
            return "accentD";
        }
        if (axis === "Y") {
            return "accentB";
        }
        return "accentE";
    }

    onAxisValueChanged: refreshDerivedState()
    onTranslationTriggerChanged: refreshPresentationState()

    Component.onCompleted: reset()
}
