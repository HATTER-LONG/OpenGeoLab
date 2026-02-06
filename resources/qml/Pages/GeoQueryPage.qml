import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../util"
import OpenGeoLab 1.0

FunctionPageBase {
    id: root

    pageTitle: qsTr("Query")
    pageIcon: "qrc:/opengeolab/resources/icons/query.svg"
    serviceName: "GeometryService"
    actionId: "query_entity"

    width: 360
    Column {
        width: parent.width
        spacing: 12

        // Entity Selector
        Selector {
            id: picker
            width: parent.width
            visible: true
        }
    }
}
