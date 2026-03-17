pragma ComponentBehavior: Bound

import QtQml

QtObject {
    function cloneValue(value) {
        if (Array.isArray(value)) {
            return value.map(function (entry) {
                return cloneValue(entry);
            });
        }
        if (value && typeof value === "object") {
            const clone = {};
            for (const key in value) {
                clone[key] = cloneValue(value[key]);
            }
            return clone;
        }
        return value;
    }

    function isNonEmptyString(value) {
        return typeof value === "string" && value.trim().length > 0;
    }

    function isObject(value) {
        return value && typeof value === "object" && !Array.isArray(value);
    }

    function isValidPathSegment(segment) {
        return isNonEmptyString(segment) && /^[A-Za-z_][A-Za-z0-9_]*$/.test(segment);
    }

    function isValidPath(path) {
        if (!Array.isArray(path) || path.length === 0) {
            return false;
        }
        for (let index = 0; index < path.length; ++index) {
            if (!isValidPathSegment(path[index])) {
                return false;
            }
        }
        return true;
    }

    function createField(key, label, defaultValue, unit, positiveOnly, accent, path) {
        return {
            "key": key,
            "label": label,
            "defaultValue": defaultValue,
            "unit": unit,
            "positiveOnly": positiveOnly,
            "accent": accent || "accentA",
            "path": path || [key]
        };
    }

    function createGeometryRequestSpec(action,
                                       shapeType,
                                       defaultName,
                                       positionTitle,
                                       positionFields,
                                       dimensionTitle,
                                       dimensionFields,
                                       axisOptions,
                                       defaultAxis) {
        return {
            "module": "geometry",
            "action": action,
            "shapeType": shapeType,
            "defaultName": defaultName,
            "positionTitle": positionTitle,
            "positionFields": positionFields,
            "dimensionTitle": dimensionTitle,
            "dimensionFields": dimensionFields,
            "axisOptions": axisOptions || [],
            "defaultAxis": defaultAxis || "Z"
        };
    }

    function createActionDefinition(key,
                                    ribbonTitle,
                                    pageTitle,
                                    sectionTitle,
                                    icon,
                                    accent,
                                    summary,
                                    nextMilestone,
                                    focusPoints,
                                    workflowKind,
                                    requestSpec) {
        return {
            "key": key,
            "ribbonTitle": ribbonTitle,
            "pageTitle": pageTitle,
            "sectionTitle": sectionTitle,
            "icon": icon,
            "accent": accent,
            "summary": summary,
            "nextMilestone": nextMilestone,
            "focusPoints": focusPoints,
            "workflowKind": workflowKind || "generic",
            "requestSpec": requestSpec === undefined ? null : requestSpec
        };
    }

    function validateField(field, fieldCollectionName, actionKey, index) {
        const errors = [];
        if (!isObject(field)) {
            errors.push(qsTr("Field %1[%2] must be an object.").arg(fieldCollectionName).arg(index));
            return errors;
        }
        if (!isNonEmptyString(field.key)) {
            errors.push(qsTr("Field %1[%2] is missing a non-empty key.").arg(fieldCollectionName).arg(index));
        }
        if (!isNonEmptyString(field.label)) {
            errors.push(qsTr("Field %1[%2] is missing a non-empty label.").arg(fieldCollectionName).arg(index));
        }
        if (!isValidPath(field.path)) {
            errors.push(qsTr("Action '%1' field %2[%3] must use a non-empty object-key path.")
                        .arg(actionKey)
                        .arg(fieldCollectionName)
                        .arg(index));
        }
        return errors;
    }

    function validateGeometryRequestSpec(actionKey, requestSpec) {
        const errors = [];
        if (!isObject(requestSpec)) {
            return {
                "errors": [qsTr("Action '%1' must provide a geometryCreate requestSpec object.")
                            .arg(actionKey)],
                "normalized": null
            };
        }

        if (!isNonEmptyString(requestSpec.module)) {
            errors.push(qsTr("Action '%1' requestSpec.module must be a non-empty string.").arg(actionKey));
        }
        if (!isNonEmptyString(requestSpec.action)) {
            errors.push(qsTr("Action '%1' requestSpec.action must be a non-empty string.").arg(actionKey));
        }
        if (!isNonEmptyString(requestSpec.shapeType)) {
            errors.push(qsTr("Action '%1' requestSpec.shapeType must be a non-empty string.").arg(actionKey));
        }
        if (!isNonEmptyString(requestSpec.defaultName)) {
            errors.push(qsTr("Action '%1' requestSpec.defaultName must be a non-empty string.").arg(actionKey));
        }
        if (!isNonEmptyString(requestSpec.positionTitle)) {
            errors.push(qsTr("Action '%1' requestSpec.positionTitle must be a non-empty string.").arg(actionKey));
        }
        if (!isNonEmptyString(requestSpec.dimensionTitle)) {
            errors.push(qsTr("Action '%1' requestSpec.dimensionTitle must be a non-empty string.").arg(actionKey));
        }
        if (!Array.isArray(requestSpec.positionFields) || requestSpec.positionFields.length === 0) {
            errors.push(qsTr("Action '%1' requestSpec.positionFields must contain at least one field.")
                        .arg(actionKey));
        }
        if (!Array.isArray(requestSpec.dimensionFields) || requestSpec.dimensionFields.length === 0) {
            errors.push(qsTr("Action '%1' requestSpec.dimensionFields must contain at least one field.")
                        .arg(actionKey));
        }

        const positionFields = Array.isArray(requestSpec.positionFields) ? requestSpec.positionFields : [];
        for (let index = 0; index < positionFields.length; ++index) {
            errors.push.apply(errors, validateField(positionFields[index], "positionFields", actionKey, index));
        }

        const dimensionFields = Array.isArray(requestSpec.dimensionFields) ? requestSpec.dimensionFields : [];
        for (let index = 0; index < dimensionFields.length; ++index) {
            errors.push.apply(errors, validateField(dimensionFields[index], "dimensionFields", actionKey, index));
        }

        const axisOptions = Array.isArray(requestSpec.axisOptions) ? requestSpec.axisOptions : [];
        for (let index = 0; index < axisOptions.length; ++index) {
            if (!isNonEmptyString(axisOptions[index])) {
                errors.push(qsTr("Action '%1' requestSpec.axisOptions[%2] must be a non-empty string.")
                            .arg(actionKey)
                            .arg(index));
            }
        }
        if (axisOptions.length > 0 && axisOptions.indexOf(requestSpec.defaultAxis) === -1) {
            errors.push(qsTr("Action '%1' requestSpec.defaultAxis must be present in axisOptions.")
                        .arg(actionKey));
        }

        return {
            "errors": errors,
            "normalized": cloneValue({
                "module": requestSpec.module,
                "action": requestSpec.action,
                "shapeType": requestSpec.shapeType,
                "defaultName": requestSpec.defaultName,
                "positionTitle": requestSpec.positionTitle,
                "positionFields": positionFields,
                "dimensionTitle": requestSpec.dimensionTitle,
                "dimensionFields": dimensionFields,
                "axisOptions": axisOptions,
                "defaultAxis": requestSpec.defaultAxis
            })
        };
    }

    function validateActionDefinition(definition) {
        const normalized = cloneValue(definition);
        const errors = [];

        if (!isNonEmptyString(normalized.key)) {
            throw new Error("ActionRegistry requires every action definition to declare a non-empty key.");
        }
        if (!isNonEmptyString(normalized.ribbonTitle)) {
            errors.push(qsTr("Action '%1' is missing a ribbonTitle.").arg(normalized.key));
        }
        if (!isNonEmptyString(normalized.pageTitle)) {
            errors.push(qsTr("Action '%1' is missing a pageTitle.").arg(normalized.key));
        }
        if (!isNonEmptyString(normalized.sectionTitle)) {
            errors.push(qsTr("Action '%1' is missing a sectionTitle.").arg(normalized.key));
        }
        if (!isNonEmptyString(normalized.icon)) {
            errors.push(qsTr("Action '%1' is missing an icon.").arg(normalized.key));
        }
        if (!isNonEmptyString(normalized.accent)) {
            errors.push(qsTr("Action '%1' is missing an accent.").arg(normalized.key));
        }
        if (!isNonEmptyString(normalized.summary)) {
            errors.push(qsTr("Action '%1' is missing a summary.").arg(normalized.key));
        }
        if (!isNonEmptyString(normalized.nextMilestone)) {
            errors.push(qsTr("Action '%1' is missing nextMilestone.").arg(normalized.key));
        }
        if (!Array.isArray(normalized.focusPoints) || normalized.focusPoints.length === 0) {
            errors.push(qsTr("Action '%1' must provide at least one focus point.").arg(normalized.key));
        }

        if (normalized.workflowKind === "generic") {
            if (normalized.requestSpec !== null) {
                errors.push(qsTr("Action '%1' uses workflowKind 'generic' and must not provide requestSpec.")
                            .arg(normalized.key));
            }
            normalized.requestSpec = null;
        } else if (normalized.workflowKind === "geometryCreate") {
            const validation = validateGeometryRequestSpec(normalized.key, normalized.requestSpec);
            errors.push.apply(errors, validation.errors);
            normalized.requestSpec = validation.normalized;
        } else {
            errors.push(qsTr("Action '%1' uses unsupported workflowKind '%2'.")
                        .arg(normalized.key)
                        .arg(normalized.workflowKind));
        }

        normalized.configError = errors.length > 0 ? errors.join(" ") : "";
        return normalized;
    }

    function buildCatalogState(rawCatalog) {
        const orderedDefinitions = [];
        const lookup = {};
        const seenKeys = {};

        for (let index = 0; index < rawCatalog.length; ++index) {
            const definition = rawCatalog[index];
            if (!definition || !isNonEmptyString(definition.key)) {
                throw new Error("ActionRegistry requires non-empty action keys in every catalog entry.");
            }
            if (seenKeys[definition.key]) {
                throw new Error("ActionRegistry duplicate action key: " + definition.key);
            }
            seenKeys[definition.key] = true;

            const validatedDefinition = validateActionDefinition(definition);
            orderedDefinitions.push(validatedDefinition);
            lookup[validatedDefinition.key] = validatedDefinition;
        }

        return {
            "orderedDefinitions": orderedDefinitions,
            "lookup": lookup
        };
    }

    readonly property var rawCatalog: [
        createActionDefinition(
            "importModel",
            qsTr("Import"),
            qsTr("Import Model"),
            qsTr("Workspace"),
            "import",
            "accentA",
            qsTr("Workflow for bringing CAD or mesh assets into the current workspace."),
            qsTr("Planned later: source selection, format options, import diagnostics, and scene insertion preview."),
            [qsTr("Source file"), qsTr("Format options"), qsTr("Import diagnostics")]
        ),
        createActionDefinition(
            "exportModel",
            qsTr("Export"),
            qsTr("Export Model"),
            qsTr("Workspace"),
            "export",
            "accentA",
            qsTr("Workflow for exporting the active model or current working selection."),
            qsTr("Planned later: export scope, target format, version options, and result logging."),
            [qsTr("Selection scope"), qsTr("Target format"), qsTr("Export log")]
        ),
        createActionDefinition(
            "toggleTheme",
            qsTr("Theme"),
            qsTr("Theme Settings"),
            qsTr("Workspace"),
            "darkTheme",
            "accentA",
            qsTr("Page for switching between engineering theme presets without interrupting work."),
            qsTr("Planned later: light and dark switching, contrast tuning, and persisted appearance preferences."),
            [qsTr("Theme preset"), qsTr("Contrast balance"), qsTr("Preference persistence")]
        ),
        createActionDefinition(
            "recordSelection",
            qsTr("Record"),
            qsTr("Script Recorder"),
            qsTr("Script Recorder"),
            "record",
            "accentB",
            qsTr("Workflow for capturing interactive actions into a reusable automation timeline."),
            qsTr("Planned later: session start/stop, event timeline review, and recorder diagnostics."),
            [qsTr("Capture actions"), qsTr("Timeline preview"), qsTr("Session notes")]
        ),
        createActionDefinition(
            "replayCommands",
            qsTr("Replay"),
            qsTr("Replay Script"),
            qsTr("Script Recorder"),
            "replay",
            "accentB",
            qsTr("Workflow for replaying previously recorded command sequences."),
            qsTr("Planned later: replay target selection, step control, and execution trace output."),
            [qsTr("Replay target"), qsTr("Step control"), qsTr("Execution trace")]
        ),
        createActionDefinition(
            "exportScript",
            qsTr("Export"),
            qsTr("Export Record"),
            qsTr("Script Recorder"),
            "exportRecord",
            "accentB",
            qsTr("Workflow for turning recorder history into a portable Python automation script."),
            qsTr("Planned later: script preview, destination selection, and export validation feedback."),
            [qsTr("Script preview"), qsTr("Destination path"), qsTr("Automation output")]
        ),
        createActionDefinition(
            "clearRecordedCommands",
            qsTr("Clear"),
            qsTr("Clear Script History"),
            qsTr("Script Recorder"),
            "clear",
            "accentD",
            qsTr("Workflow for cleaning the recorder history while keeping the rest of the workspace intact."),
            qsTr("Planned later: clear-scope confirmation, snapshot backup, and post-clear recorder state."),
            [qsTr("History scope"), qsTr("Safety check"), qsTr("Recorder reset")]
        ),
        createActionDefinition(
            "focusViewport",
            qsTr("Focus"),
            qsTr("Focus Viewport"),
            qsTr("Viewport Utilities"),
            "eye",
            "accentE",
            qsTr("Workflow for framing the active scene context and surfacing the current viewport summary."),
            qsTr("Planned later: camera focus targets, framing presets, and context-sensitive viewport feedback."),
            [qsTr("View framing"), qsTr("Selection focus"), qsTr("Camera state")]
        ),
        createActionDefinition(
            "inspectPayload",
            qsTr("Inspect"),
            qsTr("Inspect Payload"),
            qsTr("Viewport Utilities"),
            "query",
            "accentE",
            qsTr("Workflow for inspecting the latest payload emitted by the controller pipeline."),
            qsTr("Planned later: structured payload tree, render packet inspection, and raw-response browsing."),
            [qsTr("Payload tree"), qsTr("Render data"), qsTr("Selection summary")]
        ),
        createActionDefinition(
            "addBox",
            qsTr("Box"),
            qsTr("Create Box"),
            qsTr("Geometry / Create"),
            "box",
            "accentA",
            qsTr("Define a box with explicit origin and X/Y/Z dimensions, then submit it through the shared geometry service pipeline."),
            qsTr("Current flow includes direct parameter editing, command recording, and Activity panel feedback."),
            [qsTr("Origin point"), qsTr("X/Y/Z dimensions"), qsTr("Activity output")],
            "geometryCreate",
            createGeometryRequestSpec(
                "createBox",
                "box",
                "Box_001",
                qsTr("Origin Point"),
                [
                    createField("originX", qsTr("X"), "0.0", qsTr("mm"), false, "accentD", ["origin", "x"]),
                    createField("originY", qsTr("Y"), "0.0", qsTr("mm"), false, "accentB", ["origin", "y"]),
                    createField("originZ", qsTr("Z"), "0.0", qsTr("mm"), false, "accentE", ["origin", "z"])
                ],
                qsTr("Dimensions"),
                [
                    createField("sizeX", qsTr("X"), "120.0", qsTr("mm"), true, "accentD", ["dimensions", "x"]),
                    createField("sizeY", qsTr("Y"), "80.0", qsTr("mm"), true, "accentB", ["dimensions", "y"]),
                    createField("sizeZ", qsTr("Z"), "60.0", qsTr("mm"), true, "accentE", ["dimensions", "z"])
                ]
            )
        ),
        createActionDefinition(
            "addCylinder",
            qsTr("Cylinder"),
            qsTr("Create Cylinder"),
            qsTr("Geometry / Create"),
            "cylinder",
            "accentA",
            qsTr("Define a cylinder from base-center placement, radius, height, and axis selection."),
            qsTr("Current flow includes direct form editing plus Activity panel logging and Python command-line support."),
            [qsTr("Base center"), qsTr("Radius / height"), qsTr("Axis selection")],
            "geometryCreate",
            createGeometryRequestSpec(
                "createCylinder",
                "cylinder",
                "Cylinder_001",
                qsTr("Base Center"),
                [
                    createField("baseCenterX", qsTr("X"), "0.0", qsTr("mm"), false, "accentD", ["baseCenter", "x"]),
                    createField("baseCenterY", qsTr("Y"), "0.0", qsTr("mm"), false, "accentB", ["baseCenter", "y"]),
                    createField("baseCenterZ", qsTr("Z"), "0.0", qsTr("mm"), false, "accentE", ["baseCenter", "z"])
                ],
                qsTr("Dimensions"),
                [
                    createField("radius", qsTr("Radius"), "40.0", qsTr("mm"), true, "accentD", ["radius"]),
                    createField("height", qsTr("Height"), "120.0", qsTr("mm"), true, "accentE", ["height"])
                ],
                ["X", "Y", "Z"],
                "Z"
            )
        ),
        createActionDefinition(
            "addSphere",
            qsTr("Sphere"),
            qsTr("Create Sphere"),
            qsTr("Geometry / Create"),
            "sphere",
            "accentA",
            qsTr("Define a sphere with center coordinates and radius in one compact engineering form."),
            qsTr("Current flow includes live metrics and Activity panel feedback after submission."),
            [qsTr("Center point"), qsTr("Radius"), qsTr("Derived metrics")],
            "geometryCreate",
            createGeometryRequestSpec(
                "createSphere",
                "sphere",
                "Sphere_001",
                qsTr("Center Point"),
                [
                    createField("centerX", qsTr("X"), "0.0", qsTr("mm"), false, "accentD", ["center", "x"]),
                    createField("centerY", qsTr("Y"), "0.0", qsTr("mm"), false, "accentB", ["center", "y"]),
                    createField("centerZ", qsTr("Z"), "0.0", qsTr("mm"), false, "accentE", ["center", "z"])
                ],
                qsTr("Radius"),
                [
                    createField("radius", qsTr("Radius"), "55.0", qsTr("mm"), true, "accentD", ["radius"])
                ]
            )
        ),
        createActionDefinition(
            "addTorus",
            qsTr("Torus"),
            qsTr("Create Torus"),
            qsTr("Geometry / Create"),
            "torus",
            "accentA",
            qsTr("Define a torus with center placement, major and minor radii, and axis selection."),
            qsTr("Current flow includes torus validity guidance, Activity logging, and command-line follow-up."),
            [qsTr("Center point"), qsTr("Major / minor radii"), qsTr("Axis selection")],
            "geometryCreate",
            createGeometryRequestSpec(
                "createTorus",
                "torus",
                "Torus_001",
                qsTr("Center Point"),
                [
                    createField("centerX", qsTr("X"), "0.0", qsTr("mm"), false, "accentD", ["center", "x"]),
                    createField("centerY", qsTr("Y"), "0.0", qsTr("mm"), false, "accentB", ["center", "y"]),
                    createField("centerZ", qsTr("Z"), "0.0", qsTr("mm"), false, "accentE", ["center", "z"])
                ],
                qsTr("Radii"),
                [
                    createField("majorRadius", qsTr("Major Radius"), "90.0", qsTr("mm"), true, "accentD", ["majorRadius"]),
                    createField("minorRadius", qsTr("Minor Radius"), "24.0", qsTr("mm"), true, "accentB", ["minorRadius"])
                ],
                ["X", "Y", "Z"],
                "Z"
            )
        ),
        createActionDefinition(
            "trim",
            qsTr("Trim"),
            qsTr("Trim Geometry"),
            qsTr("Geometry / Modify"),
            "trim",
            "accentD",
            qsTr("Workflow for trimming geometry against selected references or bounds."),
            qsTr("Planned later: target selection, trimming options, preview, and undo-friendly command execution."),
            [qsTr("Target selection"), qsTr("Trim options"), qsTr("Undo support")]
        ),
        createActionDefinition(
            "offset",
            qsTr("Offset"),
            qsTr("Offset Geometry"),
            qsTr("Geometry / Modify"),
            "offset",
            "accentD",
            qsTr("Workflow for offsetting geometry entities while keeping topology intent visible."),
            qsTr("Planned later: offset distance input, direction control, preview, and command replay integration."),
            [qsTr("Offset distance"), qsTr("Direction control"), qsTr("Preview result")]
        ),
        createActionDefinition(
            "queryGeometry",
            qsTr("Query"),
            qsTr("Geometry Query"),
            qsTr("Geometry / Inspect"),
            "query",
            "accentE",
            qsTr("Workflow for querying geometry properties, topology, and engineering metadata."),
            qsTr("Planned later: entity picking, property tables, and structured result panels."),
            [qsTr("Entity picking"), qsTr("Property table"), qsTr("Result panel")]
        ),
        createActionDefinition(
            "generateMesh",
            qsTr("Generate"),
            qsTr("Generate Mesh"),
            qsTr("Mesh / Generate"),
            "mesh",
            "accentB",
            qsTr("Workflow for launching a meshing task from the current geometry context."),
            qsTr("Planned later: mesh size controls, algorithm presets, progress feedback, and mesh result preview."),
            [qsTr("Mesh size"), qsTr("Algorithm preset"), qsTr("Progress feedback")]
        ),
        createActionDefinition(
            "smoothMesh",
            qsTr("Smooth"),
            qsTr("Smooth Mesh"),
            qsTr("Mesh / Generate"),
            "smoothMesh",
            "accentB",
            qsTr("Workflow for smoothing an existing mesh while preserving useful quality information."),
            qsTr("Planned later: smoothing strategy selection, iteration control, and before/after quality feedback."),
            [qsTr("Smoothing strategy"), qsTr("Iteration control"), qsTr("Quality feedback")]
        ),
        createActionDefinition(
            "queryMesh",
            qsTr("Query"),
            qsTr("Mesh Query"),
            qsTr("Mesh / Inspect"),
            "query",
            "accentC",
            qsTr("Workflow for inspecting mesh statistics, element quality, and mesh-level metadata."),
            qsTr("Planned later: mesh picking, quality metrics, and issue-focused diagnostic panels."),
            [qsTr("Mesh picking"), qsTr("Quality metrics"), qsTr("Diagnostic panel")]
        ),
        createActionDefinition(
            "aiSuggest",
            qsTr("Suggest"),
            qsTr("AI Suggest"),
            qsTr("AI / Assist"),
            "aiSuggest",
            "accentE",
            qsTr("Workflow for AI-generated next-step suggestions inside the current engineering context."),
            qsTr("Planned later: context-aware suggestions, intent shortcuts, and guided workflow recommendations."),
            [qsTr("Context hints"), qsTr("Intent shortcuts"), qsTr("Workflow recommendations")]
        ),
        createActionDefinition(
            "aiChat",
            qsTr("Chat"),
            qsTr("AI Chat"),
            qsTr("AI / Assist"),
            "aiChat",
            "accentE",
            qsTr("Workflow for conversational AI assistance embedded into the workbench."),
            qsTr("Planned later: threaded chat history, model context injection, and command-oriented responses."),
            [qsTr("Threaded history"), qsTr("Model context"), qsTr("Actionable replies")]
        )
    ]

    readonly property var catalogState: buildCatalogState(rawCatalog)
    readonly property var actionDefinitions: catalogState.orderedDefinitions
    readonly property var actionLookup: catalogState.lookup

    function action(actionKey) {
        const definition = actionLookup[actionKey];
        return definition ? cloneValue(definition) : null;
    }
}
