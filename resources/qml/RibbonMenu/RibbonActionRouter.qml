pragma ComponentBehavior: Bound
import QtQuick
import OpenGeoLab 1.0

QtObject {
    id: root

    // 这里定义“应用级意图”（而不是 UI 按钮本身）
    // 后续如果接 C++ 控制器/命令系统，也只需要在这里改映射。
    signal exitApp

    function handle(actionId, payload): void {
        const fn = root._handlers[actionId];
        if (fn) {
            fn(payload);
            return;
        }
        console.warn("[RibbonActionRouter] Unhandled action:", actionId, "payload:", payload);
    }

    // 轻量映射表：避免 Main.qml 里不断膨胀 switch/case
    // 需要新增按钮时：
    // 1) RibbonConfig.qml 里加一个 item.id
    // 2) 这里加一条映射，或者新增一个 signal
    property var _handlers: ({
            "exitApp": function (_payload) {
                root.exitApp();
            },
            "toggleTheme": function (_payload) {
                Theme.mode = (Theme.mode === Theme.dark) ? Theme.light : Theme.dark;
            }
            // "importModel": function(payload) { root.importModelRequested(payload); }
        })
}
