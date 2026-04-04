import QtQuick
import QtQuick.Controls
import theme

/**
 * Left-panel tree view displaying module → action hierarchy.
 * Emits actionSelected(moduleName, actionName) when the user
 * clicks an action item, or moduleSelected(moduleName) for modules.
 */
Item {
    id: root

    required property var model
    signal actionSelected(string moduleName, string actionName)
    signal moduleSelected(string moduleName)

    Rectangle {
        anchors.fill: parent
        color: PluginTheme.surface
        radius: PluginTheme.radiusSmall

        TreeView {
            id: treeView
            anchors.fill: parent
            anchors.margins: PluginTheme.gapTight
            model: root.model
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            selectionModel: ItemSelectionModel {
                model: treeView.model
            }

            delegate: TreeViewDelegate {
                id: treeDelegate

                implicitHeight: 32
                implicitWidth: treeView.width

                contentItem: RowLayout {
                    spacing: PluginTheme.gapTight

                    Label {
                        text: treeDelegate.model.display ?? ""
                        color: treeDelegate.current
                               ? PluginTheme.textOnAccent
                               : PluginTheme.textPrimary
                        font.pixelSize: 13
                        font.weight: treeDelegate.hasChildren
                                     ? Font.DemiBold : Font.Normal
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                background: Rectangle {
                    color: {
                        if (treeDelegate.current)
                            return PluginTheme.accentA;
                        if (treeDelegate.hovered)
                            return PluginTheme.tint(
                                PluginTheme.surfaceStrong,
                                PluginTheme.darkMode ? 0.6 : 0.8
                            );
                        return "transparent";
                    }
                    radius: PluginTheme.radiusSmall / 2

                    Behavior on color {
                        ColorAnimation { duration: PluginTheme.animFast }
                    }
                }

                // Role IDs matching SchemaTreeModel (Qt.UserRole + 1, + 2).
                readonly property int moduleNameRole: 0x101
                readonly property int actionNameRole: 0x102

                onClicked: {
                    let idx = treeView.index(row, column);
                    treeView.selectionModel.setCurrentIndex(
                        idx,
                        ItemSelectionModel.ClearAndSelect
                    );

                    let mod = treeView.model.data(idx, moduleNameRole);
                    let act = treeView.model.data(idx, actionNameRole);

                    if (act !== undefined && act !== null) {
                        root.actionSelected(mod, act);
                    } else if (mod !== undefined && mod !== null) {
                        root.moduleSelected(mod);
                    }
                }
            }
        }
    }

    Label {
        anchors.centerIn: parent
        text: qsTr("No modules loaded")
        color: PluginTheme.textTertiary
        font.pixelSize: 13
        visible: root.model ? root.model.rowCount() === 0 : true
    }
}
