import QtQuick
import QtQuick.Dialogs
import OpenGeoLab 1.0 as OGL

FileDialog {
    id: dialog

    title: qsTr("Import 3D Model")
    nameFilters: [qsTr("3D Models (*.stp *.step *.brep)"), qsTr("All Files (*)")]

    /// Handle file selection and trigger backend import
    onAccepted: {
        const filePath = selectedFile.toString().replace("file:///", "");
        console.log("[ImportModel] Selected model file:", filePath);

        OGL.BackendService.request("ReaderService", JSON.stringify({
            "file_path": filePath
        }));
    }

    onRejected: {}
}
