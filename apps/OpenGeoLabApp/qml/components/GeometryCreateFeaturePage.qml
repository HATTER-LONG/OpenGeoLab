pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

FeaturePageBase {
    id: page

    required property var appController
    property var actionDefinition: null
    property bool requestPending: false
    property int queuedExecutionId: 0
    property int pendingRequestId: -1

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
        const currentActionKey = page.actionDefinition && page.actionDefinition.key
            ? page.actionDefinition.key
            : "";
        const nextActionKey = nextActionDefinition && nextActionDefinition.key
            ? nextActionDefinition.key
            : "";
        page.actionDefinition = nextActionDefinition;
        if (currentActionKey.length > 0 && currentActionKey === nextActionKey) {
            pageState.refreshPresentationState();
            return;
        }
        pageState.reset();
    }

    function fieldAccentColor(field) {
        return page.theme.resolveAccentColor(field && field.accent ? field.accent : page.accentName);
    }

    function presentAction(nextActionDefinition) {
        page.actionDefinition = nextActionDefinition;
        queuedExecutionId += 1;
        requestPending = false;
        pendingRequestId = -1;
        pageState.reset();
        present();
    }

    function executeGeometryCreate() {
        if (requestPending) {
            return;
        }

        const result = pageState.buildValidatedRequest();
        if (!result.success) {
            pageState.invalidFieldKey = result.fieldKey;
            pageState.validationMessage = result.message;
            return;
        }

        const requestJson = JSON.stringify(result.request);
        const executionId = queuedExecutionId + 1;
        queuedExecutionId = executionId;
        page.requestPending = true;
        pageState.clearValidation();

        Qt.callLater(function () {
            if (page.queuedExecutionId !== executionId) {
                return;
            }

            const requestId = appController.submitServiceRequest(requestJson);
            if (page.queuedExecutionId !== executionId || requestId < 0) {
                page.requestPending = false;
                pageState.validationMessage = appController.lastSummary && appController.lastSummary.length > 0
                    ? appController.lastSummary
                    : qsTr("Geometry create request failed.");
                return;
            }
            page.pendingRequestId = requestId;
        });
    }

    onExecuteRequested: executeGeometryCreate()
    onCancelRequested: {
        pageState.clearValidation();
        page.requestPending = false;
        page.pendingRequestId = -1;
        page.queuedExecutionId += 1;
    }

    Connections {
        target: page.appController

        function onServiceRequestFinished(requestId, success) {
            if (requestId !== page.pendingRequestId) {
                return;
            }

            page.requestPending = false;
            page.pendingRequestId = -1;
            if (success) {
                return;
            }

            pageState.validationMessage = page.appController.lastSummary && page.appController.lastSummary.length > 0
                ? page.appController.lastSummary
                : qsTr("Geometry create request failed.");
        }
    }

    GeometryCreatePageState {
        id: pageState
        objectName: "geometryCreatePageState"

        actionDefinition: page.actionDefinition
    }

    StatusMessageBanner {
        Layout.fillWidth: true
        theme: page.theme
        message: pageState.validationMessage
        badgeText: qsTr("Validation")
        accentColor: page.theme.accentD
    }

    StatusMessageBanner {
        Layout.fillWidth: true
        theme: page.theme
        message: pageState.advisoryMessage
        badgeText: qsTr("Advisory")
        accentColor: page.theme.accentC
    }

    GeometryCreateShapePage {
        Layout.fillWidth: true
        theme: page.theme
        pageState: pageState
    }

    GeometryCreatePlacementPage {
        Layout.fillWidth: true
        theme: page.theme
        pageState: pageState
    }

    GeometryCreateOrientationSection {
        Layout.fillWidth: true
        theme: page.theme
        pageState: pageState
    }

    GeometryCreateMetricsSection {
        Layout.fillWidth: true
        theme: page.theme
        pageState: pageState
    }
}
