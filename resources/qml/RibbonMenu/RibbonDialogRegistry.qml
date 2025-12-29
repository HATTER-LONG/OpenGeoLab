pragma ComponentBehavior: Bound
import QtQuick
import "../Pages/Dialogs" as PagesDialogs

QtObject {
    id: root

    function titleFor(actionId): string {
        switch (actionId) {
        case "addPoint":
            return qsTr("Add Point");
        case "addLine":
            return qsTr("Add Line");
        case "addBox":
            return qsTr("Add Box");
        case "trim":
            return qsTr("Trim");
        case "offset":
            return qsTr("Offset");
        case "generateMesh":
            return qsTr("Generate Mesh");
        case "smoothMesh":
            return qsTr("Smooth Mesh");
        case "aiSuggest":
            return qsTr("AI Suggest");
        case "aiChat":
            return qsTr("AI Chat");
        default:
            return "";
        }
    }

    function componentFor(actionId) {
        switch (actionId) {
        case "addPoint":
            return addPointDialog;
        case "addLine":
            return addLineDialog;
        case "addBox":
            return addBoxDialog;
        case "trim":
            return trimDialog;
        case "offset":
            return offsetDialog;
        case "generateMesh":
            return generateMeshDialog;
        case "smoothMesh":
            return smoothMeshDialog;
        case "aiSuggest":
            return aiSuggestDialog;
        case "aiChat":
            return aiChatDialog;
        default:
            return null;
        }
    }

    // One Component per action (simple and explicit; adding a new page is just adding a file + a case).
    readonly property Component addPointDialog: Component {
        PagesDialogs.AddPointDialog {}
    }
    readonly property Component addLineDialog: Component {
        PagesDialogs.AddLineDialog {}
    }
    readonly property Component addBoxDialog: Component {
        PagesDialogs.AddBoxDialog {}
    }
    readonly property Component trimDialog: Component {
        PagesDialogs.TrimDialog {}
    }
    readonly property Component offsetDialog: Component {
        PagesDialogs.OffsetDialog {}
    }

    readonly property Component generateMeshDialog: Component {
        PagesDialogs.GenerateMeshDialog {}
    }
    readonly property Component smoothMeshDialog: Component {
        PagesDialogs.SmoothMeshDialog {}
    }

    readonly property Component aiSuggestDialog: Component {
        PagesDialogs.AISuggestDialog {}
    }
    readonly property Component aiChatDialog: Component {
        PagesDialogs.AIChatDialog {}
    }
}
