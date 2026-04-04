# Action Debugger QML Rewrite — Design Spec

## Motivation

The PySide6 widget-based Action Debugger UI does not match the main
application's visual quality.  The main app uses QML with a comprehensive
theme system (`AppTheme.qml`), custom components, and animations.  Rewriting
the plugin UI in QML ensures visual consistency, reuses the proven design
language, and produces an embeddable component ready for the future AI Chat
window.

## Design Decisions (from brainstorm)

| Decision               | Choice                                                |
| ---------------------- | ----------------------------------------------------- |
| Theme strategy         | Copy a simplified `PluginTheme.qml` to plugin dir     |
| Window mode            | `ActionDebuggerPage.qml` is an `Item`, not a window   |
| Param form             | Python `ParamListModel` + QML `Repeater` rendering    |
| JSON syntax highlight  | Python `QSyntaxHighlighter` attached to QML TextArea  |
| Icons                  | SVGs in `plugins/_shared/icons/`, loaded via file URL  |

## File Structure

```
plugins/ai_chat_plugin/
├── __init__.py                  # describe_plugin() + launch_ui()
├── __main__.py                  # Standalone entry (QML ApplicationWindow)
├── scene_tools.py               # Cached 4-layer schema API (unchanged)
├── schema_tree_model.py         # QStandardItemModel (unchanged)
├── json_highlighter.py          # QSyntaxHighlighter (add set_theme, unchanged)
├── debugger_backend.py          # NEW: DebuggerBackend(QObject) controller
├── param_list_model.py          # NEW: ParamListModel(QAbstractListModel)
├── qml/
│   ├── ActionDebuggerPage.qml   # Root embeddable component
│   ├── ActionDebuggerWindow.qml # Standalone window wrapper
│   ├── SchemaTreeView.qml       # Left panel — module/action tree
│   ├── DetailPanel.qml          # Top right — JSON schema display
│   ├── ParamForm.qml            # Middle right — dynamic param input
│   ├── RequestResponseView.qml  # Bottom right — request + response
│   └── theme/
│       └── PluginTheme.qml      # Singleton — dark/light color palette
│
plugins/_shared/
├── icons/                       # SVG icons (already copied)
│   ├── lightTheme.svg
│   ├── darkTheme.svg
│   ├── play.svg
│   ├── clear.svg
│   └── ... (21 icons total)
└── plugin_theme.py              # Python theme (kept for non-QML plugins)
```

### Files removed after migration

Once the QML UI is verified, the following PySide6 widget files are deleted:

- `action_debugger.py`  (replaced by `debugger_backend.py` + QML)
- `param_form_builder.py`  (replaced by `param_list_model.py` + `ParamForm.qml`)

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│  QML Layer (rendering + interaction)                    │
│  ActionDebuggerPage.qml                                 │
│  ├─ SchemaTreeView.qml          ── binds ──┐            │
│  ├─ DetailPanel.qml              ── binds ──┤            │
│  ├─ ParamForm.qml                ── binds ──┤            │
│  └─ RequestResponseView.qml     ── binds ──┤            │
│                                             │            │
│  PluginTheme (singleton)                    │            │
└─────────────────────────────────────────────┼────────────┘
                                              │
                     Q_PROPERTY / Q_INVOKABLE │
                                              │
┌─────────────────────────────────────────────┼────────────┐
│  Python Layer (data + logic)                │            │
│  DebuggerBackend(QObject)       ◄───────────┘            │
│  ├─ schemaTreeModel: SchemaTreeModel                     │
│  ├─ paramListModel: ParamListModel                       │
│  ├─ scene_tools (module-level functions)                  │
│  └─ JsonHighlighter (attached to QML TextArea docs)      │
└──────────────────────────────────────────────────────────┘
```

**Data flow:**

1. `DebuggerBackend` populates `SchemaTreeModel` from `scene_tools` at init.
2. QML `SchemaTreeView` displays the model; user clicks an action.
3. QML calls `backend.selectAction(moduleName, actionName)`.
4. Backend fetches the schema via `scene_tools.describe_action()`, updates
   `detailJson`, rebuilds `paramListModel`, updates `requestJson`.
5. QML binds reactively: `DetailPanel` shows schema, `ParamForm` renders
   fields, `RequestResponseView` shows request JSON.
6. User edits params in form → `ParamListModel.value` role changes →
   backend re-serialises `requestJson`.
7. User clicks Execute → QML calls `backend.execute()`.
8. Backend spawns worker thread, updates `progress` and `isExecuting`.
9. Worker completes → backend sets `responseJson` → QML displays result.

## Component Details

### 1. PluginTheme.qml

A `pragma Singleton` QML object exposing color properties for dark and light
modes.  Mirrors the subset of `AppTheme.qml` used by the plugin.

```qml
pragma Singleton
import QtQuick

QtObject {
    property bool darkMode: true

    // Backgrounds
    readonly property color bg:             darkMode ? "#0a0d11" : "#f9fbfd"
    readonly property color surface:        darkMode ? "#10151b" : "#ffffff"
    readonly property color surfaceMuted:   darkMode ? "#151c24" : "#eef3f8"
    readonly property color surfaceStrong:  darkMode ? "#1d2630" : "#dfe9f4"

    // Text
    readonly property color textPrimary:    darkMode ? "#f4f7fb" : "#16283c"
    readonly property color textSecondary:  darkMode ? "#a0acb9" : "#60748b"
    readonly property color textOnAccent:   "#ffffff"

    // Accent
    readonly property color accentA:        darkMode ? "#5aa2ff" : "#1473e6"
    readonly property color accentB:        darkMode ? "#85c0ff" : "#14ae8a"

    // Status
    readonly property color success:        darkMode ? "#6fe3b0" : "#1f9d68"
    readonly property color danger:         darkMode ? "#ff8d7d" : "#d9534f"
    readonly property color warning:        darkMode ? "#ffd071" : "#d89209"

    // Border
    readonly property color borderSubtle:   darkMode ? "#27313c" : "#d6e0eb"

    // Dimensions
    readonly property int radiusSmall:  12
    readonly property int radiusMedium: 18
    readonly property int gapTight:     8
    readonly property int gap:          12
    readonly property int gapWide:      16

    // Typography
    readonly property string monoFont: "Consolas"
}
```

The `darkMode` property is writable so the backend can set it via
`PluginTheme.darkMode = backend.isDark`.  The backend calls this in its
`toggleTheme()` implementation.  All dependent colors update reactively.

A `qmldir` file in `qml/theme/` registers the singleton:

```
singleton PluginTheme 1.0 PluginTheme.qml
```

The QML engine import path includes the plugin's `qml/` directory, so QML
files import it as `import theme`.

### 2. DebuggerBackend (Python)

```python
class DebuggerBackend(QObject):
    # ── Properties ────────────────────────────────────────
    currentModule    = Property(str, ...)       # selected module name
    currentAction    = Property(str, ...)       # selected action name
    detailJson       = Property(str, ...)       # schema JSON for detail panel
    requestJson      = Property(str, ...)       # full request JSON (auto-built)
    responseJson     = Property(str, ...)       # execution result JSON
    isExecuting      = Property(bool, ...)      # True while worker is running
    progress         = Property(float, ...)     # 0.0–1.0
    progressMessage  = Property(str, ...)       # progress text
    isDark           = Property(bool, ...)      # dark/light mode toggle

    # ── Models (exposed as context properties) ────────────
    schemaTreeModel  : SchemaTreeModel          # module→action hierarchy
    paramListModel   : ParamListModel           # current action params

    # ── Q_INVOKABLE ──────────────────────────────────────
    def selectAction(module: str, action: str)  # update detail + params
    def execute()                                # run action on worker thread
    def clear()                                  # reset form + response
    def toggleTheme()                            # flip isDark
    def updateParamValue(name: str, value)       # called by QML on form edit
```

**Key behaviours:**

- `selectAction`: calls `scene_tools.describe_action()`, rebuilds
  `paramListModel`, serialises initial `requestJson`.
- `execute`: reads `requestJson` text (user may have edited it), spawns
  worker thread, posts progress/result via `QMetaObject.invokeMethod`.
- `updateParamValue`: called by QML delegates when user edits a field;
  updates the model value and re-serialises `requestJson`.
- `toggleTheme`: flips `isDark` property; QML `PluginTheme.darkMode` is
  bound to it, triggering reactive color updates.

### 3. ParamListModel (Python)

A `QAbstractListModel` exposing action parameters as a flat list for QML
`Repeater` consumption.

**Roles:**

| Role          | Type   | Description                              |
| ------------- | ------ | ---------------------------------------- |
| `name`        | str    | Parameter name                           |
| `paramType`   | str    | `"string"`, `"number"`, `"boolean"`, etc |
| `required`    | bool   | Whether the parameter is required        |
| `description` | str    | Tooltip / placeholder text               |
| `value`       | var    | Current value (edited by QML)            |
| `enabled`     | bool   | For optional params: checkbox state      |

**API:**

```python
def load_from_schema(params_dict: dict)   # rebuild from action schema
def set_value(index: int, value)          # called by QML delegate
def set_enabled(index: int, enabled: bool)
def build_params() -> dict                # collect enabled params into dict
def clear()                               # reset all to defaults
```

### 4. ActionDebuggerPage.qml (Root component)

The main embeddable component.  Layout: horizontal `SplitView`.

```
required property var backend

SplitView {
    orientation: Qt.Horizontal

    SchemaTreeView {
        model: backend.schemaTreeModel
        onActionSelected: (module, action) => backend.selectAction(module, action)
        SplitView.preferredWidth: 240
        SplitView.minimumWidth: 180
    }

    SplitView {
        orientation: Qt.Vertical

        DetailPanel {
            json: backend.detailJson
            SplitView.preferredHeight: parent.height * 0.3
        }

        ParamForm {
            model: backend.paramListModel
            onValueChanged: (name, value) => backend.updateParamValue(name, value)
            onExecuteClicked: backend.execute()
            onClearClicked: backend.clear()
            isExecuting: backend.isExecuting
            progress: backend.progress
        }

        RequestResponseView {
            requestJson: backend.requestJson
            responseJson: backend.responseJson
            onRequestEdited: (text) => backend.requestJson = text
        }
    }
}
```

### 5. SchemaTreeView.qml

Uses `TreeView` (Qt 6.4+) with `SelectionModel`.  Each item displays the
module/action name; selected item has accent background.  Items use
`MODULE_NAME_ROLE` and `ACTION_NAME_ROLE` from the Python model.

### 6. DetailPanel.qml

A read-only `ScrollView` + `TextArea` displaying the action schema JSON.
The Python `JsonHighlighter` is attached to its `textDocument` from the
backend after the QML component is created.

### 7. ParamForm.qml

A `Column` with:

- A `Repeater` bound to `paramListModel`.
- Delegate: `Loader` that selects a component based on `paramType`:
  - `"string"` → `TextField`
  - `"number"` → `TextField` with `validator: DoubleValidator`
  - `"integer"` → `TextField` with `validator: IntValidator`
  - `"boolean"` → `CheckBox`
  - `"array"` / `"object"` → `TextArea` (compact, 3 lines)
- For `required === false` items: a leading `CheckBox` controls `enabled`;
  when unchecked the delegate is visually dimmed and its value is excluded.
- Below the form: a row with Execute button (▶ icon from `play.svg`),
  Clear button (`clear.svg` icon), progress bar, and progress text.

### 8. RequestResponseView.qml

A horizontal `SplitView` with two panels:

- **Left — Request:** Editable `TextArea` with `JsonHighlighter` attached.
  Displays `backend.requestJson`.  When user edits, emits `requestEdited`.
  The edit does NOT sync back to the param form.
- **Right — Response:** Read-only `TextArea` bound to `backend.responseJson`.

Both panels have a "Request" / "Response" label above them.

### 9. ActionDebuggerWindow.qml (Standalone wrapper)

For standalone mode only.  An `ApplicationWindow` wrapping the page:

```qml
ApplicationWindow {
    width: 1000; height: 700
    title: qsTr("Action Debugger — AI Chat Plugin")
    color: PluginTheme.bg

    header: ToolBar {
        RowLayout {
            Label { text: qsTr("Action Debugger"); font.bold: true }
            Item { Layout.fillWidth: true }
            ToolButton {
                icon.source: backend.isDark
                    ? "../../_shared/icons/darkTheme.svg"
                    : "../../_shared/icons/lightTheme.svg"
                onClicked: backend.toggleTheme()
            }
        }
    }

    ActionDebuggerPage { backend: _backend; anchors.fill: parent }
}
```

## Standalone vs Hosted Mode

| Aspect            | Standalone (`__main__.py`)          | Hosted (`launch_ui()`)             |
| ----------------- | ----------------------------------- | ---------------------------------- |
| QApplication      | Created by `__main__.py`            | Already exists (C++ host)          |
| pywrapper         | Loaded via `_setup_standalone_paths` | Already imported by host           |
| QML root          | `ActionDebuggerWindow.qml`          | `ActionDebuggerWindow.qml`         |
| Schema tree       | Populated (modules available)       | Populated (modules available)      |
| Theme detection   | Defaults to dark                    | Detects from host palette          |

Both modes use the same `DebuggerBackend` + QML stack.  The only difference
is QApplication creation and pywrapper path setup.

## JSON Highlighting Integration

The `JsonHighlighter` is a `QSyntaxHighlighter` subclass.  After the QML
engine creates a `TextArea`, the backend finds it via `objectName` and
attaches a highlighter to its `textDocument`:

```python
# In DebuggerBackend, after QML loaded:
detail_area = engine.rootObjects()[0].findChild(QObject, "detailTextArea")
self._detail_hl = JsonHighlighter(detail_area.property("textDocument"), theme=...)
```

When the theme toggles, `hl.set_theme(new_theme)` is called.

## Icon Usage

Icons are loaded from `plugins/_shared/icons/` via file URLs.  The backend
exposes an `iconPath` property pointing to the resolved directory:

```python
@Property(str, constant=True)
def iconPath(self):
    return Path(__file__).resolve().parent.parent.joinpath("_shared", "icons").as_uri()
```

QML uses:
```qml
icon.source: backend.iconPath + "/play.svg"
```

## Files Deleted After Migration

- `plugins/ai_chat_plugin/action_debugger.py`
- `plugins/ai_chat_plugin/param_form_builder.py`
- `plugins/_shared/plugin_theme.py` — kept (used by other plugins), but
  no longer imported by ai_chat_plugin.

## Scope Boundary

This spec covers the QML rewrite of the **Action Debugger** component only
(Segment 1 scope).  The Chat UI (Segment 2) will reuse `PluginTheme.qml`,
`DebuggerBackend` patterns, and the `JsonHighlighter` integration, but is
a separate spec.
