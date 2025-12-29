pragma Singleton
import QtQuick
import QtQuick.Dialogs

FileDialog {
    id: dialog

    title: qsTr("Import 3D Model")
    nameFilters: [qsTr("3D Models (*.stp *.step *.brep)"), qsTr("All Files (*)")]

    onAccepted: {
        console.log("[ImportModel] Selected model file:", selectedFile);
    }

    onRejected: {}
}
