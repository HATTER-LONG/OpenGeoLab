pragma ComponentBehavior: Bound
import QtQuick
import "." as Dialogs

Dialogs.ActionParamsDialog {
    actionId: "addPoint"
    title: qsTr("Add Point")

    fields: ([
            {
                key: "x",
                label: qsTr("X"),
                type: "number",
                placeholder: qsTr("0"),
                default: 0
            },
            {
                key: "y",
                label: qsTr("Y"),
                type: "number",
                placeholder: qsTr("0"),
                default: 0
            },
            {
                key: "z",
                label: qsTr("Z"),
                type: "number",
                placeholder: qsTr("0"),
                default: 0
            }
        ])
}
