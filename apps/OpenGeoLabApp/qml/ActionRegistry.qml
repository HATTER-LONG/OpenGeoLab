pragma ComponentBehavior: Bound

import QtQml

QtObject {
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

    function createGeometryRequestSpec(action, shapeType, defaultName, positionTitle, positionFields, dimensionTitle, dimensionFields, axisOptions, defaultAxis) {
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

    function createAction(pageTitle, sectionTitle, icon, accent, summary, nextMilestone, focusPoints, workflowKind, requestSpec) {
        return {
            "pageTitle": pageTitle,
            "sectionTitle": sectionTitle,
            "icon": icon,
            "accent": accent,
            "summary": summary,
            "nextMilestone": nextMilestone,
            "focusPoints": focusPoints,
            "workflowKind": workflowKind || "generic",
            "requestSpec": requestSpec || null
        };
    }

    readonly property var actionDefinitions: ({
        "importModel": createAction(
            qsTr("Import Model"),
            qsTr("Workspace"),
            "import",
            "accentA",
            qsTr("Workflow for bringing CAD or mesh assets into the current workspace."),
            qsTr("Planned later: source selection, format options, import diagnostics, and scene insertion preview."),
            [qsTr("Source file"), qsTr("Format options"), qsTr("Import diagnostics")]
        ),
        "exportModel": createAction(
            qsTr("Export Model"),
            qsTr("Workspace"),
            "export",
            "accentA",
            qsTr("Workflow for exporting the active model or current working selection."),
            qsTr("Planned later: export scope, target format, version options, and result logging."),
            [qsTr("Selection scope"), qsTr("Target format"), qsTr("Export log")]
        ),
        "toggleTheme": createAction(
            qsTr("Theme Settings"),
            qsTr("Workspace"),
            "darkTheme",
            "accentA",
            qsTr("Page for switching between engineering theme presets without interrupting work."),
            qsTr("Planned later: light and dark switching, contrast tuning, and persisted appearance preferences."),
            [qsTr("Theme preset"), qsTr("Contrast balance"), qsTr("Preference persistence")]
        ),
        "recordSelection": createAction(
            qsTr("Script Recorder"),
            qsTr("Script Recorder"),
            "record",
            "accentB",
            qsTr("Workflow for capturing interactive actions into a reusable automation timeline."),
            qsTr("Planned later: session start/stop, event timeline review, and recorder diagnostics."),
            [qsTr("Capture actions"), qsTr("Timeline preview"), qsTr("Session notes")]
        ),
        "replayCommands": createAction(
            qsTr("Replay Script"),
            qsTr("Script Recorder"),
            "replay",
            "accentB",
            qsTr("Workflow for replaying previously recorded command sequences."),
            qsTr("Planned later: replay target selection, step control, and execution trace output."),
            [qsTr("Replay target"), qsTr("Step control"), qsTr("Execution trace")]
        ),
        "exportScript": createAction(
            qsTr("Export Record"),
            qsTr("Script Recorder"),
            "exportRecord",
            "accentB",
            qsTr("Workflow for turning recorder history into a portable Python automation script."),
            qsTr("Planned later: script preview, destination selection, and export validation feedback."),
            [qsTr("Script preview"), qsTr("Destination path"), qsTr("Automation output")]
        ),
        "clearRecordedCommands": createAction(
            qsTr("Clear Script History"),
            qsTr("Script Recorder"),
            "clear",
            "accentD",
            qsTr("Workflow for cleaning the recorder history while keeping the rest of the workspace intact."),
            qsTr("Planned later: clear-scope confirmation, snapshot backup, and post-clear recorder state."),
            [qsTr("History scope"), qsTr("Safety check"), qsTr("Recorder reset")]
        ),
        "focusViewport": createAction(
            qsTr("Focus Viewport"),
            qsTr("Viewport Utilities"),
            "eye",
            "accentE",
            qsTr("Workflow for framing the active scene context and surfacing the current viewport summary."),
            qsTr("Planned later: camera focus targets, framing presets, and context-sensitive viewport feedback."),
            [qsTr("View framing"), qsTr("Selection focus"), qsTr("Camera state")]
        ),
        "inspectPayload": createAction(
            qsTr("Inspect Payload"),
            qsTr("Viewport Utilities"),
            "query",
            "accentE",
            qsTr("Workflow for inspecting the latest payload emitted by the controller pipeline."),
            qsTr("Planned later: structured payload tree, render packet inspection, and raw-response browsing."),
            [qsTr("Payload tree"), qsTr("Render data"), qsTr("Selection summary")]
        ),
        "addBox": createAction(
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
        "addCylinder": createAction(
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
        "addSphere": createAction(
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
        "addTorus": createAction(
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
        "trim": createAction(
            qsTr("Trim Geometry"),
            qsTr("Geometry / Modify"),
            "trim",
            "accentD",
            qsTr("Workflow for trimming geometry against selected references or bounds."),
            qsTr("Planned later: target selection, trimming options, preview, and undo-friendly command execution."),
            [qsTr("Target selection"), qsTr("Trim options"), qsTr("Undo support")]
        ),
        "offset": createAction(
            qsTr("Offset Geometry"),
            qsTr("Geometry / Modify"),
            "offset",
            "accentD",
            qsTr("Workflow for offsetting geometry entities while keeping topology intent visible."),
            qsTr("Planned later: offset distance input, direction control, preview, and command replay integration."),
            [qsTr("Offset distance"), qsTr("Direction control"), qsTr("Preview result")]
        ),
        "queryGeometry": createAction(
            qsTr("Geometry Query"),
            qsTr("Geometry / Inspect"),
            "query",
            "accentE",
            qsTr("Workflow for querying geometry properties, topology, and engineering metadata."),
            qsTr("Planned later: entity picking, property tables, and structured result panels."),
            [qsTr("Entity picking"), qsTr("Property table"), qsTr("Result panel")]
        ),
        "generateMesh": createAction(
            qsTr("Generate Mesh"),
            qsTr("Mesh / Generate"),
            "mesh",
            "accentB",
            qsTr("Workflow for launching a meshing task from the current geometry context."),
            qsTr("Planned later: mesh size controls, algorithm presets, progress feedback, and mesh result preview."),
            [qsTr("Mesh size"), qsTr("Algorithm preset"), qsTr("Progress feedback")]
        ),
        "smoothMesh": createAction(
            qsTr("Smooth Mesh"),
            qsTr("Mesh / Generate"),
            "smoothMesh",
            "accentB",
            qsTr("Workflow for smoothing an existing mesh while preserving useful quality information."),
            qsTr("Planned later: smoothing strategy selection, iteration control, and before/after quality feedback."),
            [qsTr("Smoothing strategy"), qsTr("Iteration control"), qsTr("Quality feedback")]
        ),
        "queryMesh": createAction(
            qsTr("Mesh Query"),
            qsTr("Mesh / Inspect"),
            "query",
            "accentC",
            qsTr("Workflow for inspecting mesh statistics, element quality, and mesh-level metadata."),
            qsTr("Planned later: mesh picking, quality metrics, and issue-focused diagnostic panels."),
            [qsTr("Mesh picking"), qsTr("Quality metrics"), qsTr("Diagnostic panel")]
        ),
        "aiSuggest": createAction(
            qsTr("AI Suggest"),
            qsTr("AI / Assist"),
            "aiSuggest",
            "accentE",
            qsTr("Workflow for AI-generated next-step suggestions inside the current engineering context."),
            qsTr("Planned later: context-aware suggestions, intent shortcuts, and guided workflow recommendations."),
            [qsTr("Context hints"), qsTr("Intent shortcuts"), qsTr("Workflow recommendations")]
        ),
        "aiChat": createAction(
            qsTr("AI Chat"),
            qsTr("AI / Assist"),
            "aiChat",
            "accentE",
            qsTr("Workflow for conversational AI assistance embedded into the workbench."),
            qsTr("Planned later: threaded chat history, model context injection, and command-oriented responses."),
            [qsTr("Threaded history"), qsTr("Model context"), qsTr("Actionable replies")]
        )
    })

    function action(actionKey) {
        const definition = actionDefinitions[actionKey];
        if (!definition) {
            return null;
        }

        return {
            "key": actionKey,
            "pageTitle": definition.pageTitle,
            "sectionTitle": definition.sectionTitle,
            "icon": definition.icon,
            "accent": definition.accent,
            "summary": definition.summary,
            "nextMilestone": definition.nextMilestone,
            "focusPoints": definition.focusPoints,
            "workflowKind": definition.workflowKind,
            "requestSpec": definition.requestSpec
        };
    }
}
