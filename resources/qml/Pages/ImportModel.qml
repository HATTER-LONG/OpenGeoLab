import QtQuick
import QtQuick.Dialogs
import OpenGeoLab 1.0 as OGL

/**
 * File dialog for importing 3D model files.
 * Triggers backend ModelReader service on file selection.
 */
FileDialog {
    id: dialog

    title: qsTr("Import 3D Model")
    nameFilters: [qsTr("3D Models (*.stp *.step *.brep)"), qsTr("All Files (*)")]

    onAccepted: {
        const filePath = selectedFile.toString().replace("file:///", "");
        console.log("[ImportModel] Selected model file:", filePath);

        // Call backend service with module routing
        OGL.BackendService.request("read_model", {
            "module": "ModelReader",
            "file_path": filePath
        });
    }

    onRejected: {}
}
