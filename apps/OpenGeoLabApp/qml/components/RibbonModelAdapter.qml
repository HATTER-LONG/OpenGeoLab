pragma ComponentBehavior: Bound

import QtQml

QtObject {
    id: adapter

    required property var ribbonConfig
    required property var actionRegistry
    required property var appController
    property string reloadToken: ""
    property var reportedWarnings: ({})

    readonly property var tabs: buildTabs()
    readonly property var groupsByTab: buildGroupsByTab()

    onReloadTokenChanged: reportedWarnings = ({})

    function buildTabs() {
        const _reloadDependency = reloadToken;
        return ribbonConfig && ribbonConfig.tabs ? ribbonConfig.tabs : [];
    }

    function buildGroupsByTab() {
        const _reloadDependency = reloadToken;

        if (!ribbonConfig || !ribbonConfig.groupsModel) {
            return [];
        }

        const result = [];
        for (let tabIndex = 0; tabIndex < ribbonConfig.groupsModel.length; ++tabIndex) {
            const groupLayouts = ribbonConfig.groupsModel[tabIndex] || [];
            const groups = [];
            for (let groupIndex = 0; groupIndex < groupLayouts.length; ++groupIndex) {
                groups.push(resolveGroup(groupLayouts[groupIndex]));
            }
            result.push(groups);
        }
        return result;
    }

    function resolveGroup(groupLayout) {
        const actions = [];
        const actionKeys = groupLayout && groupLayout.actionKeys ? groupLayout.actionKeys : [];

        for (let index = 0; index < actionKeys.length; ++index) {
            const actionKey = actionKeys[index];
            const actionDefinition = actionRegistry ? actionRegistry.action(actionKey) : null;
            if (!actionDefinition) {
                postMissingActionWarning(actionKey);
                continue;
            }

            actions.push({
                "key": actionDefinition.key,
                "title": actionDefinition.ribbonTitle || actionDefinition.pageTitle,
                "icon": actionDefinition.icon,
                "accentOne": actionDefinition.accent,
                "summary": actionDefinition.summary,
                "workflowKind": actionDefinition.workflowKind,
                "configError": actionDefinition.configError || ""
            });
        }

        return {
            "title": groupLayout && groupLayout.title ? groupLayout.title : "",
            "actions": actions
        };
    }

    function postMissingActionWarning(actionKey) {
        if (!appController || !actionKey || reportedWarnings[actionKey]) {
            return;
        }

        const nextWarnings = {};
        for (const key in reportedWarnings) {
            nextWarnings[key] = reportedWarnings[key];
        }
        nextWarnings[actionKey] = true;
        reportedWarnings = nextWarnings;

        appController.postUiNotice(
            3,
            "Qml.RibbonModelAdapter",
            qsTr("Ribbon action skipped"),
            qsTr("RibbonConfig references an unknown action key: %1.").arg(actionKey)
        );
    }

    function groupsForTab(tabIndex) {
        const _reloadDependency = reloadToken;

        if (tabIndex < 0 || tabIndex >= groupsByTab.length) {
            return [];
        }

        return groupsByTab[tabIndex];
    }
}
