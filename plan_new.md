## Plan: 几何拾取与 SelectManagerService 完整实现（DRAFT）

TL;DR — 在 C++ 层新增 SelectManagerService（QML 可用的中间层），增强 SelectManager 的父类展开逻辑（Part/Solid 递归到根并将其所有面加入高亮），更新渲染与视口以高效显示批量高亮，重构现有 QML (Selector.qml / GeoQueryPage.qml) 以使用 Service，补充注释、文档与单元/集成测试。这样可把选择逻辑封装、提升性能并保持 QML 简洁一致。

Steps

1. 设计与接口（短期先行草案）

新建接口头：include/app/select_manager_service.hpp，实现文件 src/app/select_manager_service.cpp。
暴露给 QML：注册为 SelectManagerService 单例/上下文属性；提供 Q_PROPERTY：
selectedEntities（list）、selectedType（string）、pickModeActive（bool）
信号：selectionChanged(), pickModeChanged(), entityPicked()。
方法：addSelection(type, uid), removeSelection(type, uid), clearSelection(), activatePickMode(type), deactivatePickMode()
在应用 QML 注册点注册服务（现有注册文件处，参考项目中 QML 注册实现）。

2. 扩展选择/父类查找逻辑（规则已确认）

修改/补充实体层查找接口以支持“向上递归到根父类”的遍历：修改或补充 include/geometry/entity/geometry_entity.hpp / geometry_entity.cpp 中的父子遍历函数，确保返回稳定且线程安全的父链。
在 SelectManager 中添加“当选中 Part/Solid 时展开父链并将父类的所有面加入选择/高亮”的策略（在 select_manager.hpp 与实现中）。
决策：当选中 Part/ Solid 时，按你的选择策略“替换当前选择”（即清除其它类型选择，仅保留展开后的面集合）。

3. 渲染与视口适配（性能优化）

在视口拾取流程中调用新的展开逻辑，更新 opengl_viewport.cpp 以在拾取事件完成后通过 SelectManagerService 发布变化。
更新渲染高亮逻辑以支持批量高亮（避免逐面单独提交），修改或增加高亮接口在 [src/render/scene_renderer.cpp]（如果文件名不同，请在渲染模块对应文件加入）。
加入简单缓存/索引（例如基于实体 UID 的面列表缓存）以避免重复遍历大型装配。

4. QML 层重构与控件完善

将 resources/qml/util/Selector.qml 重构为使用 SelectManagerService 替代对 PickManager 的直接调用，暴露信号绑定到组件信号（参考你已提供的 Selector.qml 文件位置: resources/qml/util/Selector.qml）。
将 resources/qml/Pages/GeoQueryPage.qml 更新为通过 Selector.qml 获取 selectedEntities，保持页面 UI，但移除对静态 Picker 的直接依赖。
强化 Selector 行为：点/边/面支持多选；solid/part 仅单选（并按上文替换选择策略）；加入可配置 visibleEntityTypes（你的 QML 已有，确认保留）。

5. 文档与注释（Doxygen 风格）

遵循 doxygen_comment_style.md 标准，对新增的 C++ 头/源（Service、SelectManager 修改、几何遍历函数、渲染扩展）补齐注释。目标文件示例位置：include/render/select_manager_service.hpp、select_manager.hpp、include/geometry/entity/geometry_entity.hpp。
更新 json_protocols.md，记录 SelectManagerService 与 QML/Backend 交互的新消息/字段。
以中文为主更新 README.md，并同步更新 README.en.md（先中文 -> 再英文）。


6. 验证与发布检查

本地验证步骤：
构建：cmake -S . -B build && cmake --build build --config Release
运行测试：ctest --test-dir build --output-on-failure
手动验证：启动应用，打开 GeoQuery 页面，选择 Part/Solid，确认 UI 中高亮替换行为并能正确查询/显示属性。
验收标准：
编译通过且相关单元测试全部通过。
在复杂装配（若有）对 Part/Solid 的选择能在 500ms 内完成父链展开和高亮（可接受阈值，可在实现中调整）。
QML Selector 与 GeoQueryPage 不再直接依赖 PickManager 静态 API（同时保留向后兼容层可选）。
Verification

构建与测试命令（本地验证）：
cmake 配置与构建（Windows PowerShell）
cmake -S . -B build -G "Ninja"
cmake --build build --config Release
运行测试
ctest --test-dir build --output-on-failure
手动交互检查：打开 UI → Pages → Query → 使用 Selector 选取 Part/Solid，观察渲染高亮与 Query 结果列表。

7.Decisions (已确认)

父类查找：向上递归查找到根父类并将其所有面加入高亮（用户选择）。
Part/Solid 选择行为：替换当前选择（清除其他选择）。
SelectManagerService：暴露属性、信号与直接操作方法（方法 + 属性/信号）。
测试范围：以单元测试为主并包含关键 QML 集成测试。