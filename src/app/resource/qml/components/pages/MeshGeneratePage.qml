import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import OpenGeoLab.Services 1.0
import "../.."
import ".."

FunctionPageBase {
    id: root

    pageTitle: qsTr("Generate Mesh")
    pageIcon: "mesh"
    actionId: "generateMesh"
    maxContentHeight: 560

    property real elementSize: 10.0
    property int meshDimension: 2
    property string elementType: "triangle"
    property string algorithm: "delaunay"
    property string sizeMode: "absolute"
    property int meshOrder: 1
    property bool optimizeMesh: false
    property bool appendMode: true

    readonly property var algorithmOptions2d: [
        { label: qsTr("Automatic"), value: "automatic" },
        { label: qsTr("MeshAdapt"), value: "meshadapt" },
        { label: qsTr("Delaunay"), value: "delaunay" },
        { label: qsTr("Frontal"), value: "frontal" },
        { label: qsTr("BAMG"), value: "bamg" },
        { label: qsTr("Frontal Quad"), value: "frontal_quad" }
    ]
    readonly property var algorithmOptions3d: [
        { label: qsTr("Delaunay"), value: "delaunay" },
        { label: qsTr("Frontal"), value: "frontal" },
        { label: qsTr("MMG3D"), value: "mmg3d" },
        { label: qsTr("RTree"), value: "rtree" },
        { label: qsTr("HXT"), value: "hxt" }
    ]
    readonly property var algorithmOptions: root.meshDimension === 3
        ? root.algorithmOptions3d
        : root.algorithmOptions2d

    onMeshDimensionChanged: {
        if (!root.algorithmSupported(root.algorithm)) {
            root.algorithm = "delaunay";
        }
    }

    function algorithmLabel(value) {
        for (var i = 0; i < root.algorithmOptions.length; i++) {
            if (root.algorithmOptions[i].value === value) {
                return root.algorithmOptions[i].label;
            }
        }
        return value;
    }

    function algorithmSupported(value) {
        for (var i = 0; i < root.algorithmOptions.length; i++) {
            if (root.algorithmOptions[i].value === value) {
                return true;
            }
        }
        return false;
    }

    function open(payload) {
        root.x = 292;
        root.y = 0;
        root.pageVisible = true;
        root.forceActiveFocus();
        sceneCommand("set_pick_mode", {
            pickMask: geoTypeSelector.mask,
            enabled: true
        });
    }

    function close() {
        sceneCommand("set_pick_mode", { enabled: false });
        sceneCommand("clear_selection", {});
        root.pageVisible = false;
        if (MainPages.currentOpenPage === root.actionId) {
            MainPages.currentOpenPage = "";
        }
    }

    function getParameters() {
        var entities = [];
        var selections = SelectionService.selections;
        for (var i = 0; i < selections.length; i++) {
            entities.push({
                shapeId: selections[i].shapeId,
                type: entityTypeTag(selections[i].entityType),
                localId: selections[i].localId
            });
        }
        return {
            module: "mesh",
            action: "generate_mesh",
            param: {
                entities: entities,
                elementSize: root.elementSize,
                sizeMode: root.sizeMode,
                dimension: root.meshDimension,
                elementType: root.elementType,
                algorithm: root.algorithm,
                advanced: {
                    order: root.meshOrder,
                    optimize: root.optimizeMesh
                },
                append: root.appendMode
            },
            mute: false
        };
    }

    function execute() {
        if (RequestService.busy || SelectionService.selections.length === 0) {
            return;
        }
        RequestService.submitAsync(JSON.stringify(root.getParameters()));
        root.close();
    }

    function sceneCommand(action, param) {
        RequestService.submitAsync(JSON.stringify({
            module: "scene",
            action: action,
            param: param ?? {},
            mute: true
        }));
    }

    function entityTypeTag(typeInt) {
        var map = {
            3: "GeoFace",
            4: "GeoSolid"
        };
        return map[typeInt] ?? "GeoFace";
    }

    Text {
        text: qsTr("Target Geometry")
        color: root.theme.textSecondary
        font.pixelSize: 12
    }

    EntityTypeSelector {
        id: geoTypeSelector

        width: parent.width
        theme: root.theme
        mask: geoTypeSelector.maskFace
        exclusiveMasks: [geoTypeSelector.maskFace, geoTypeSelector.maskSolid]
        typeModel: [
            { label: qsTr("Face"),  icon: "entityFace",  mask: geoTypeSelector.maskFace },
            { label: qsTr("Solid"), icon: "entitySolid", mask: geoTypeSelector.maskSolid }
        ]

        onMaskChanged: {
            sceneCommand("set_pick_mode", { pickMask: geoTypeSelector.mask });
        }
    }

    Rectangle {
        width: parent.width
        height: 24
        radius: root.theme.radiusSmall
        color: SelectionService.pickEnabled
            ? root.theme.tint(root.theme.accentA, root.theme.darkMode ? 0.14 : 0.08)
            : "transparent"
        visible: SelectionService.pickEnabled

        Behavior on color {
            ColorAnimation { duration: root.theme.animNormal }
        }

        Row {
            anchors.centerIn: parent
            spacing: 6

            Rectangle {
                width: 6
                height: 6
                radius: 3
                anchors.verticalCenter: parent.verticalCenter
                color: root.theme.accentA

                SequentialAnimation on opacity {
                    running: SelectionService.pickEnabled
                    loops: Animation.Infinite

                    NumberAnimation {
                        from: 1.0
                        to: 0.3
                        duration: 800
                        easing.type: Easing.InOutQuad
                    }

                    NumberAnimation {
                        from: 0.3
                        to: 1.0
                        duration: 800
                        easing.type: Easing.InOutQuad
                    }
                }
            }

            Text {
                text: qsTr("Click faces or solids in the viewport")
                color: root.theme.accentA
                font.pixelSize: 10
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    Text {
        text: qsTr("Selected: %1").arg(SelectionService.selections.length)
        color: root.theme.textSecondary
        font.pixelSize: 12
        visible: SelectionService.selections.length > 0
    }

    Flickable {
        id: chipFlickable

        width: parent.width
        height: Math.min(chipFlow.implicitHeight, 80)
        contentHeight: chipFlow.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height
        visible: SelectionService.selections.length > 0

        Flow {
            id: chipFlow

            width: chipFlickable.width
            spacing: 6

            Repeater {
                model: SelectionService.selections

                EntityChip {
                    required property var modelData

                    theme: root.theme
                    shapeId: modelData.shapeId
                    entityType: modelData.entityType
                    localId: modelData.localId

                    onRemoveRequested: function(sid, etype, lid) {
                        sceneCommand("deselect", {
                            entities: [{ shapeId: sid, type: root.entityTypeTag(etype), localId: lid }]
                        });
                    }
                }
            }
        }
    }

    Rectangle {
        width: parent.width
        height: 40
        radius: root.theme.radiusSmall
        color: root.theme.surfaceMuted
        visible: SelectionService.selections.length === 0

        Text {
            anchors.centerIn: parent
            text: qsTr("No geometry selected.\nPick faces or solids from the viewport.")
            color: root.theme.textTertiary
            font.pixelSize: 11
            horizontalAlignment: Text.AlignHCenter
            lineHeight: 1.3
        }
    }

    Text {
        text: qsTr("Mesh Parameters")
        color: root.theme.textSecondary
        font.pixelSize: 12
    }

    RowLayout {
        width: parent.width
        spacing: 6

        Row {
            spacing: 4

            Repeater {
                model: [
                    { label: qsTr("Absolute"), value: "absolute" },
                    { label: "%", value: "percentage" }
                ]

                delegate: Rectangle {
                    required property var modelData

                    width: modelData.value === "percentage" ? 30 : 62
                    height: 24
                    radius: root.theme.radiusSmall
                    color: root.sizeMode === modelData.value
                        ? root.theme.tint(root.theme.accentB, root.theme.darkMode ? 0.24 : 0.14)
                        : root.theme.surfaceMuted
                    border.width: root.sizeMode === modelData.value ? 1.5 : 1
                    border.color: root.sizeMode === modelData.value
                        ? root.theme.accentB
                        : root.theme.borderSubtle

                    Text {
                        anchors.centerIn: parent
                        text: parent.modelData.label
                        font.pixelSize: 10
                        font.bold: root.sizeMode === parent.modelData.value
                        color: root.sizeMode === parent.modelData.value
                            ? root.theme.accentB
                            : root.theme.textSecondary
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.sizeMode = parent.modelData.value
                    }
                }
            }
        }

        DimensionInput {
            Layout.fillWidth: true
            theme: root.theme
            label: qsTr("Size")
            value: root.elementSize
            accentColor: root.theme.accentB
            tooltipText: root.sizeMode === "percentage"
                ? qsTr("Percentage of bounding box diagonal")
                : qsTr("Target element size")

            onValueEdited: function(newVal) {
                root.elementSize = newVal;
            }
        }
    }

    RowLayout {
        width: parent.width
        spacing: 6

        Text {
            text: qsTr("Dimension")
            color: root.theme.textSecondary
            font.pixelSize: 12
            Layout.fillWidth: true
        }

        Row {
            spacing: 4

            Repeater {
                model: [
                    { label: qsTr("2D"), value: 2 },
                    { label: qsTr("3D"), value: 3 }
                ]

                delegate: Rectangle {
                    required property var modelData

                    width: 36
                    height: 24
                    radius: root.theme.radiusSmall
                    color: root.meshDimension === modelData.value
                        ? root.theme.tint(root.theme.accentB, root.theme.darkMode ? 0.24 : 0.14)
                        : root.theme.surfaceMuted
                    border.width: root.meshDimension === modelData.value ? 1.5 : 1
                    border.color: root.meshDimension === modelData.value
                        ? root.theme.accentB
                        : root.theme.borderSubtle

                    Text {
                        anchors.centerIn: parent
                        text: parent.modelData.label
                        font.pixelSize: 10
                        font.bold: root.meshDimension === parent.modelData.value
                        color: root.meshDimension === parent.modelData.value
                            ? root.theme.accentB
                            : root.theme.textSecondary
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.meshDimension = parent.modelData.value
                    }
                }
            }
        }
    }

    RowLayout {
        width: parent.width
        spacing: 6

        Text {
            text: qsTr("Element")
            color: root.theme.textSecondary
            font.pixelSize: 12
            Layout.fillWidth: true
        }

        Row {
            spacing: 4

            Repeater {
                model: [
                    { label: qsTr("Tri"), value: "triangle" },
                    { label: qsTr("Quad"), value: "quad" }
                ]

                delegate: Rectangle {
                    required property var modelData

                    width: 44
                    height: 24
                    radius: root.theme.radiusSmall
                    color: root.elementType === modelData.value
                        ? root.theme.tint(root.theme.accentB, root.theme.darkMode ? 0.24 : 0.14)
                        : root.theme.surfaceMuted
                    border.width: root.elementType === modelData.value ? 1.5 : 1
                    border.color: root.elementType === modelData.value
                        ? root.theme.accentB
                        : root.theme.borderSubtle

                    Text {
                        anchors.centerIn: parent
                        text: parent.modelData.label
                        font.pixelSize: 10
                        font.bold: root.elementType === parent.modelData.value
                        color: root.elementType === parent.modelData.value
                            ? root.theme.accentB
                            : root.theme.textSecondary
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.elementType = parent.modelData.value
                    }
                }
            }
        }
    }

    Column {
        width: parent.width
        spacing: 6

        Text {
            text: qsTr("Algorithm")
            color: root.theme.textSecondary
            font.pixelSize: 12
        }

        Rectangle {
            width: parent.width
            height: algorithmInfoCol.implicitHeight + 16
            radius: root.theme.radiusSmall
            color: root.theme.surfaceMuted

            Column {
                id: algorithmInfoCol

                anchors.fill: parent
                anchors.margins: 8
                spacing: 4

                Text {
                    id: algorithmSummary

                    width: parent.width
                    text: qsTr("Selected algorithm: %1").arg(root.algorithmLabel(root.algorithm))
                    color: root.theme.textPrimary
                    font.pixelSize: 11
                    font.bold: true
                    wrapMode: Text.WordWrap
                }

                Text {
                    width: parent.width
                    text: root.meshDimension === 3
                        ? qsTr("3D mesh generation supports Delaunay, Frontal, MMG3D, RTree and HXT.")
                        : qsTr("2D mesh generation supports Automatic, MeshAdapt, Delaunay, Frontal, BAMG and Frontal Quad.")
                    color: root.theme.textSecondary
                    font.pixelSize: 10
                    wrapMode: Text.WordWrap
                }
            }
        }

        Flow {
            width: parent.width
            spacing: 6

            Repeater {
                model: root.algorithmOptions

                delegate: Rectangle {
                    required property var modelData

                    width: algorithmText.implicitWidth + 18
                    height: 26
                    radius: 13
                    color: root.algorithm === modelData.value
                        ? root.theme.tint(root.theme.accentC, root.theme.darkMode ? 0.24 : 0.14)
                        : root.theme.surfaceMuted
                    border.width: root.algorithm === modelData.value ? 1.5 : 1
                    border.color: root.algorithm === modelData.value
                        ? root.theme.accentC
                        : root.theme.borderSubtle

                    Text {
                        id: algorithmText

                        anchors.centerIn: parent
                        text: parent.modelData.label
                        font.pixelSize: 10
                        font.bold: root.algorithm === parent.modelData.value
                        color: root.algorithm === parent.modelData.value
                            ? root.theme.accentC
                            : root.theme.textSecondary
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.algorithm = parent.modelData.value
                    }
                }
            }
        }
    }

    RowLayout {
        width: parent.width
        spacing: 6

        Text {
            text: qsTr("Order")
            color: root.theme.textSecondary
            font.pixelSize: 12
            Layout.fillWidth: true
        }

        Row {
            spacing: 4

            Repeater {
                model: [
                    { label: qsTr("Linear"), value: 1 },
                    { label: qsTr("Quadratic"), value: 2 }
                ]

                delegate: Rectangle {
                    required property var modelData

                    width: modelData.value === 2 ? 72 : 50
                    height: 24
                    radius: root.theme.radiusSmall
                    color: root.meshOrder === modelData.value
                        ? root.theme.tint(root.theme.accentB, root.theme.darkMode ? 0.24 : 0.14)
                        : root.theme.surfaceMuted
                    border.width: root.meshOrder === modelData.value ? 1.5 : 1
                    border.color: root.meshOrder === modelData.value
                        ? root.theme.accentB
                        : root.theme.borderSubtle

                    Text {
                        anchors.centerIn: parent
                        text: parent.modelData.label
                        font.pixelSize: 10
                        font.bold: root.meshOrder === parent.modelData.value
                        color: root.meshOrder === parent.modelData.value
                            ? root.theme.accentB
                            : root.theme.textSecondary
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.meshOrder = parent.modelData.value
                    }
                }
            }
        }
    }

    RowLayout {
        width: parent.width
        spacing: 8

        Text {
            text: qsTr("Optimize")
            color: root.theme.textSecondary
            font.pixelSize: 12
            Layout.fillWidth: true
        }

        Switch {
            checked: root.optimizeMesh
            onToggled: root.optimizeMesh = checked
        }
    }

    RowLayout {
        width: parent.width
        spacing: 8

        Text {
            text: qsTr("Append")
            color: root.theme.textSecondary
            font.pixelSize: 12
            Layout.fillWidth: true
        }

        Switch {
            checked: root.appendMode
            onToggled: root.appendMode = checked
        }

        Text {
            text: qsTr("Merge with existing mesh")
            color: root.theme.textTertiary
            font.pixelSize: 10
        }
    }

    Rectangle {
        width: parent.width
        height: helperText.implicitHeight + 16
        radius: root.theme.radiusSmall
        color: root.theme.surfaceMuted

        Text {
            id: helperText

            anchors.fill: parent
            anchors.margins: 8
            text: SelectionService.selections.length === 0
                ? qsTr("Pick at least one face or solid before running mesh generation.")
                : qsTr("Generate Mesh uses the current selection and closes this panel after submitting the request.")
            color: SelectionService.selections.length === 0 ? root.theme.warning : root.theme.textSecondary
            font.pixelSize: 10
            wrapMode: Text.WordWrap
        }
    }

    Rectangle {
        width: parent.width
        height: 32
        radius: root.theme.radiusSmall
        color: clearMeshMouse.pressed
            ? root.theme.surfaceStrong
            : (clearMeshMouse.containsMouse
                ? root.theme.tint(root.theme.surfaceMuted, root.theme.darkMode ? 0.96 : 0.9)
                : root.theme.surfaceMuted)
        border.width: 1
        border.color: clearMeshMouse.containsMouse
            ? root.theme.tint(root.theme.danger, root.theme.darkMode ? 0.56 : 0.36)
            : root.theme.borderSubtle

        Behavior on color {
            ColorAnimation { duration: root.theme.animFast }
        }

        Text {
            anchors.centerIn: parent
            text: qsTr("Clear All Mesh")
            color: clearMeshMouse.containsMouse ? root.theme.danger : root.theme.textPrimary
            font.pixelSize: 12
            font.bold: true
        }

        MouseArea {
            id: clearMeshMouse

            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                RequestService.submitAsync(JSON.stringify({
                    module: "mesh",
                    action: "clear_mesh",
                    param: {},
                    mute: false
                }));
            }
        }
    }
}
