pragma ComponentBehavior: Bound

import QtQml

QtObject {
    id: adapter

    required property var appController
    property string fallbackSummary: ""
    readonly property string defaultSummary: qsTr("Viewport is active. Ribbon commands stay connected to the same controller pipeline.")
    readonly property string summaryText: resolveSummaryText()

    function isObject(value) {
        return value && typeof value === "object" && !Array.isArray(value);
    }

    function trimmedString(value) {
        return typeof value === "string" ? value.trim() : "";
    }

    function parseResponsePreview() {
        if (!appController || !trimmedString(appController.lastPayload).length) {
            return {};
        }

        try {
            return JSON.parse(appController.lastPayload);
        } catch (error) {
            return {};
        }
    }

    function pluralizedBodies(bodyCount) {
        return bodyCount === 1 ? qsTr("1 body") : qsTr("%1 bodies").arg(bodyCount);
    }

    function summarizeModel(model) {
        if (!isObject(model) || !trimmedString(model.modelName).length) {
            return "";
        }

        if (typeof model.bodyCount === "number") {
            return qsTr("Model %1 is ready with %2.")
                .arg(model.modelName)
                .arg(pluralizedBodies(model.bodyCount));
        }

        return qsTr("Model %1 is ready.").arg(model.modelName);
    }

    function summarizePayload(payload) {
        if (!isObject(payload)) {
            return "";
        }

        const explicitSummary = trimmedString(payload.summary);
        if (explicitSummary.length > 0) {
            return explicitSummary;
        }

        const modelSummary = summarizeModel(payload.model);
        if (modelSummary.length > 0) {
            return modelSummary;
        }

        const geometrySummary = summarizeModel(payload.geometryModel);
        if (geometrySummary.length > 0) {
            return geometrySummary;
        }

        if (isObject(payload.renderFrame) && trimmedString(payload.renderFrame.frameId).length > 0) {
            return qsTr("Render frame %1 is ready with %2 draw items.")
                .arg(payload.renderFrame.frameId)
                .arg(payload.renderFrame.drawItemCount || 0);
        }

        if (isObject(payload.sceneGraph) && trimmedString(payload.sceneGraph.sceneId).length > 0) {
            return qsTr("Scene %1 is ready with %2 nodes.")
                .arg(payload.sceneGraph.sceneId)
                .arg(payload.sceneGraph.nodeCount || 0);
        }

        if (isObject(payload.selectionResult) && trimmedString(payload.selectionResult.mode).length > 0) {
            return qsTr("Selection mode %1 returned %2 hits.")
                .arg(payload.selectionResult.mode)
                .arg(payload.selectionResult.hitCount || 0);
        }

        if (isObject(payload.replayReport)) {
            return qsTr("Replayed %1 commands with %2 successes.")
                .arg(payload.replayReport.replayedCount || 0)
                .arg(payload.replayReport.successCount || 0);
        }

        if (typeof payload.recordedCommandCount === "number") {
            return qsTr("Recorder now holds %1 commands.").arg(payload.recordedCommandCount);
        }

        return "";
    }

    function resolveSummaryText() {
        const responsePreview = parseResponsePreview();
        const payload = isObject(responsePreview.payload) ? responsePreview.payload : {};
        const payloadSummary = summarizePayload(payload);
        const currentSummary = trimmedString(appController ? appController.lastSummary : "");
        const payloadDeclaredSummary = trimmedString(payload.summary);
        const responseMessage = trimmedString(responsePreview.message);

        if (payloadSummary.length > 0 &&
                (currentSummary.length === 0 ||
                 currentSummary === payloadDeclaredSummary ||
                 currentSummary === responseMessage)) {
            return payloadSummary;
        }

        const fallback = trimmedString(fallbackSummary);
        if (fallback.length > 0) {
            return fallback;
        }

        if (currentSummary.length > 0) {
            return currentSummary;
        }

        return defaultSummary;
    }
}
