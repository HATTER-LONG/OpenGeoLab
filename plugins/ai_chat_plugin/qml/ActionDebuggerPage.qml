import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Root embeddable component for the Action Debugger.
 * Assembles SchemaTreeView, DetailPanel, ParamForm and
 * RequestResponseView into a two-level SplitView layout.
 */
Item {
    id: root

    required property var backend

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        SchemaTreeView {
            model: root.backend.schemaTreeModel
            SplitView.preferredWidth: 240
            SplitView.minimumWidth: 180

            onActionSelected: (moduleName, actionName) => {
                root.backend.selectAction(moduleName, actionName);
            }
            onModuleSelected: (moduleName) => {
                root.backend.selectAction(moduleName, "");
            }
        }

        SplitView {
            orientation: Qt.Vertical
            SplitView.fillWidth: true

            DetailPanel {
                json: root.backend.detailJson
                SplitView.preferredHeight: 200
                SplitView.minimumHeight: 100
            }

            ParamForm {
                model: root.backend.paramListModel
                isExecuting: root.backend.isExecuting
                progress: root.backend.progress
                SplitView.preferredHeight: 240
                SplitView.minimumHeight: 120

                onExecuteClicked: root.backend.execute()
                onClearClicked: root.backend.clear()
            }

            RequestResponseView {
                requestJson: root.backend.requestJson
                responseJson: root.backend.responseJson
                SplitView.fillHeight: true
                SplitView.minimumHeight: 120

                onRequestEdited: (text) => {
                    root.backend.requestJson = text;
                }
            }
        }
    }
}
