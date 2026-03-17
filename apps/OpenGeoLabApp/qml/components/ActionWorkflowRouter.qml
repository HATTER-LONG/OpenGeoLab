pragma ComponentBehavior: Bound

import QtQml

QtObject {
    id: router

    required property var actionRegistry
    required property var appController
    required property var actionFeaturePage
    required property var geometryCreateFeaturePage
    property string currentActionKey: ""
    property string currentWorkflowKind: ""
    readonly property string noticeSource: "Qml.ActionWorkflowRouter"

    function isNonEmptyString(value) {
        return typeof value === "string" && value.trim().length > 0;
    }

    function isObject(value) {
        return value && typeof value === "object" && !Array.isArray(value);
    }

    function hasOpenPage() {
        return (actionFeaturePage && actionFeaturePage.open) ||
            (geometryCreateFeaturePage && geometryCreateFeaturePage.open);
    }

    function closePages() {
        if (actionFeaturePage) {
            actionFeaturePage.open = false;
        }
        if (geometryCreateFeaturePage) {
            geometryCreateFeaturePage.open = false;
        }
    }

    function reportUnknownAction(actionKey) {
        if (!appController) {
            return;
        }

        appController.postUiNotice(
            4,
            noticeSource,
            qsTr("Unknown action"),
            qsTr("Action metadata was not registered for key: %1.").arg(actionKey)
        );
    }

    function reportConfigurationWarning(actionKey, detail) {
        if (!appController) {
            return;
        }

        appController.postUiNotice(
            3,
            noticeSource,
            qsTr("Action configuration warning"),
            detail && detail.length > 0
                ? detail
                : qsTr("Action '%1' has an invalid workflow configuration.").arg(actionKey)
        );
    }

    function validateGeometryRequestSpec(actionDefinition) {
        const requestSpec = actionDefinition.requestSpec;
        if (!isObject(requestSpec)) {
            return qsTr("Action '%1' must provide a complete geometryCreate requestSpec.")
                .arg(actionDefinition.key);
        }
        if (!isNonEmptyString(requestSpec.module) || !isNonEmptyString(requestSpec.action) ||
                !isNonEmptyString(requestSpec.shapeType) ||
                !isNonEmptyString(requestSpec.defaultName) ||
                !isNonEmptyString(requestSpec.positionTitle) ||
                !isNonEmptyString(requestSpec.dimensionTitle)) {
            return qsTr("Action '%1' must provide a complete geometryCreate requestSpec.")
                .arg(actionDefinition.key);
        }
        if (!Array.isArray(requestSpec.positionFields) || requestSpec.positionFields.length === 0 ||
                !Array.isArray(requestSpec.dimensionFields) || requestSpec.dimensionFields.length === 0) {
            return qsTr("Action '%1' must provide a complete geometryCreate requestSpec.")
                .arg(actionDefinition.key);
        }
        return "";
    }

    function validateActionDefinition(actionDefinition) {
        if (!actionDefinition) {
            return qsTr("Action metadata is not available.");
        }
        if (isNonEmptyString(actionDefinition.configError)) {
            return actionDefinition.configError;
        }
        if (actionDefinition.workflowKind === "generic") {
            return "";
        }
        if (actionDefinition.workflowKind === "geometryCreate") {
            return validateGeometryRequestSpec(actionDefinition);
        }
        return qsTr("Action '%1' uses unsupported workflowKind '%2'.")
            .arg(actionDefinition.key)
            .arg(actionDefinition.workflowKind);
    }

    function presentActionDefinition(actionDefinition) {
        if (actionDefinition.workflowKind === "geometryCreate") {
            geometryCreateFeaturePage.presentAction(actionDefinition);
            currentWorkflowKind = "geometryCreate";
            return;
        }

        actionFeaturePage.presentAction(actionDefinition);
        currentWorkflowKind = "generic";
    }

    function openAction(actionKey) {
        const actionDefinition = actionRegistry ? actionRegistry.action(actionKey) : null;
        if (!actionDefinition) {
            closePages();
            currentActionKey = "";
            currentWorkflowKind = "";
            reportUnknownAction(actionKey);
            return false;
        }

        const validationError = validateActionDefinition(actionDefinition);
        if (validationError.length > 0) {
            closePages();
            currentActionKey = "";
            currentWorkflowKind = "";
            reportConfigurationWarning(actionKey, validationError);
            return false;
        }

        closePages();
        presentActionDefinition(actionDefinition);
        currentActionKey = actionDefinition.key;
        return true;
    }

    function refreshOpenAction() {
        if (!currentActionKey || !hasOpenPage()) {
            return false;
        }

        const actionDefinition = actionRegistry ? actionRegistry.action(currentActionKey) : null;
        if (!actionDefinition) {
            closePages();
            reportUnknownAction(currentActionKey);
            currentActionKey = "";
            currentWorkflowKind = "";
            return false;
        }

        const validationError = validateActionDefinition(actionDefinition);
        if (validationError.length > 0) {
            closePages();
            reportConfigurationWarning(currentActionKey, validationError);
            currentActionKey = "";
            currentWorkflowKind = "";
            return false;
        }

        if (geometryCreateFeaturePage && geometryCreateFeaturePage.open) {
            geometryCreateFeaturePage.refreshAction(actionDefinition);
            currentWorkflowKind = "geometryCreate";
            return true;
        }

        if (actionFeaturePage && actionFeaturePage.open) {
            actionFeaturePage.refreshAction(actionDefinition);
            currentWorkflowKind = "generic";
            return true;
        }

        return false;
    }
}
