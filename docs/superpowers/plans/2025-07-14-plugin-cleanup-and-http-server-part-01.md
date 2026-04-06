# Plugin Cleanup & HTTP Server Plugin — Part 1 of 2

> Part 文件：包含 Task 1（删除 demo 插件）和 Task 2（隐藏 AI Ribbon Tab）的完整任务与代码片段，保证自足可读。

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **Recommended models:** Use Claude Opus 4.6 for design or plan revisions. Use GPT-5.4 for implementation, debugging, verification, and code-review subtasks.

**Goal:** Remove 4 demo plugins and hide the AI ribbon tab to clean up the production UI.

**Architecture:** Delete demo plugin files directly (plugin discovery is dynamic via `pkgutil.iter_modules`, no registration code to update). Modify `RibbonConfig.qml` to remove the AI tab entry and its groups, then update `AppHeader.qml` pluginTabIndex. Remove the `aiChat` special handler in `Main.qml` since the plugin is already accessible via the generic `pluginUI_*` path.

**Tech Stack:** QML (Qt 6.9), Python plugins

**Spec:** `docs/superpowers/specs/2025-07-14-plugin-cleanup-and-http-server-design.md`

---

## File Structure

| Action | File | Responsibility |
|--------|------|---------------|
| Delete | `plugins/hello_plugin.py` | Minimal script plugin demo |
| Delete | `plugins/progress_demo_plugin.py` | Progress callback demo |
| Delete | `plugins/demo_ui_plugin/` (directory) | PySide6 window demo |
| Delete | `plugins/selection_demo_plugin/` (directory) | Selection workflow demo |
| Modify | `src/app/resource/qml/RibbonConfig.qml` | Remove AI tab and its groups |
| Modify | `src/app/resource/qml/sections/AppHeader.qml:22` | Update `pluginTabIndex` from 3 → 2 |
| Modify | `src/app/resource/qml/Main.qml:100-113` | Remove `aiChat` special handler |

---

### Task 1: Remove 4 Demo Plugins

**Files:**
- Delete: `plugins/hello_plugin.py`
- Delete: `plugins/progress_demo_plugin.py`
- Delete: `plugins/demo_ui_plugin/` (entire directory)
- Delete: `plugins/selection_demo_plugin/` (entire directory)

- [ ] **Step 1: Delete all 4 demo plugins**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
Remove-Item -Recurse -Force plugins\hello_plugin.py
Remove-Item -Recurse -Force plugins\progress_demo_plugin.py
Remove-Item -Recurse -Force plugins\demo_ui_plugin
Remove-Item -Recurse -Force plugins\selection_demo_plugin
```

- [ ] **Step 2: Verify only production files remain**

```powershell
Get-ChildItem plugins -Exclude __pycache__ | Select-Object Name
```

Expected output — exactly these entries:
```
_shared
ai_chat_plugin
```

- [ ] **Step 3: Verify no references to deleted plugins exist in the codebase**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
git --no-pager grep -r "hello_plugin\|progress_demo_plugin\|demo_ui_plugin\|selection_demo_plugin" -- ":(exclude)docs/" ":(exclude)*.md"
```

Expected: no output (no code references; documentation references are acceptable).

- [ ] **Step 4: Run existing Python tests to confirm no breakage**

```powershell
.\pyvenv\Scripts\python.exe -m pytest plugins\ai_chat_plugin\tests -q
```

Expected: all tests pass (no demo plugin dependency).

- [ ] **Step 5: Commit**

```powershell
git add -A plugins/hello_plugin.py plugins/progress_demo_plugin.py plugins/demo_ui_plugin plugins/selection_demo_plugin
git commit -m "chore(plugins): remove 4 demo plugins

Remove hello_plugin.py, progress_demo_plugin.py, demo_ui_plugin/,
and selection_demo_plugin/. These were development-only demos not
intended for production. Plugin discovery is dynamic via pkgutil,
so no registration code needs updating.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 2: Hide AI Ribbon Tab

**Files:**
- Modify: `src/app/resource/qml/RibbonConfig.qml`
- Modify: `src/app/resource/qml/sections/AppHeader.qml:22`
- Modify: `src/app/resource/qml/Main.qml:100-113`

**Context:** The ribbon tab system works as follows:
- `RibbonConfig.qml` defines `tabs` (array of tab names) and `groupsModel` (array of arrays, one per tab, containing action groups)
- `AppHeader.qml` reads `ribbonTabs` and `ribbonGroups` from the config; it has a `pluginTabIndex` property (currently `3`) to detect when to show the `PluginRibbonGroup` instead of normal ribbon groups
- `Main.qml` has a special `aiChat` handler that launches `ai_chat_plugin` by name; this is redundant because the plugin already has `hasUI: true` and appears in the Plugins tab via the generic `pluginUI_*` dispatch path

- [ ] **Step 1: Remove AI tab from RibbonConfig.qml**

Edit `src/app/resource/qml/RibbonConfig.qml` — remove `qsTr("AI")` from the `tabs` array:

```qml
// Before:
readonly property var tabs: [qsTr("Geometry"), qsTr("Mesh"), qsTr("AI"), qsTr("Plugins")]

// After:
readonly property var tabs: [qsTr("Geometry"), qsTr("Mesh"), qsTr("Plugins")]
```

- [ ] **Step 2: Remove AI groups from groupsModel in RibbonConfig.qml**

Edit `src/app/resource/qml/RibbonConfig.qml` — remove the 3rd element in `groupsModel` (the AI Assist group at index 2, lines 104-123) and keep the empty `[]` for Plugins as the new index 2:

```qml
// Before (lines 104-124):
    ], [
            {
                "title": qsTr("Assist"),
                "actions": [
                    {
                        "key": "aiSuggest",
                        "title": qsTr("Suggest"),
                        "icon": "aiSuggest",
                        "accentOne": "accentE",
                        "accentTwo": "accentA"
                    },
                    {
                        "key": "aiChat",
                        "title": qsTr("Chat"),
                        "icon": "aiChat",
                        "accentOne": "accentE",
                        "accentTwo": "accentA"
                    }
                ]
            }
        ], []]

// After:
    ], []]
```

- [ ] **Step 3: Update pluginTabIndex in AppHeader.qml**

Edit `src/app/resource/qml/sections/AppHeader.qml` line 22:

```qml
// Before:
property int pluginTabIndex: 3

// After:
property int pluginTabIndex: 2
```

- [ ] **Step 4: Remove aiChat special handler from Main.qml**

Edit `src/app/resource/qml/Main.qml` — remove lines 100-113 (the `aiChat` handler block):

```qml
// Remove this entire block:
        // AI Chat — launch via plugin system.
        if (actionKey === "aiChat") {
            root.statusNote = qsTr("Launching AI Chat…");
            root.menuOpen = false;
            RequestService.executeOnMainThread(JSON.stringify({
                module: "plugins",
                action: "invoke_ui",
                param: {
                    pluginName: "ai_chat_plugin"
                },
                mute: true
            }));
            return;
        }
```

The `ai_chat_plugin` has `hasUI: true` in its `describe_plugin()`, so it already appears in the Plugins tab. When clicked, `PluginRibbonGroup.qml` generates `actionKey = "pluginUI_ai_chat_plugin"`, which hits the existing `pluginUI_*` handler at lines 116-129.

- [ ] **Step 5: Verify the build succeeds**

```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_app --parallel 4
```

Expected: build succeeds (QML is loaded at runtime, but ensuring the C++ app target builds confirms no resource errors).

- [ ] **Step 6: Run full test suite to confirm no breakage**

```powershell
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

Expected: all existing tests pass.

- [ ] **Step 7: Commit**

```powershell
git add src/app/resource/qml/RibbonConfig.qml src/app/resource/qml/sections/AppHeader.qml src/app/resource/qml/Main.qml
git commit -m "refactor(app): hide AI ribbon tab

Remove the AI tab from the ribbon bar. The ai_chat_plugin remains
fully functional and accessible via the Plugins tab (hasUI: true).

Changes:
- RibbonConfig.qml: remove AI tab and Assist groups
- AppHeader.qml: update pluginTabIndex from 3 to 2
- Main.qml: remove redundant aiChat special handler

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```
