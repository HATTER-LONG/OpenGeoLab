# Activity Log & Command Panel — Design Spec

## 1. Overview

Add a bottom-right floating overlay that provides:

- **Activity button**: toggles the panel open/closed, shows notification badge
- **Activity Panel** with two tabs:
  - **Events**: structured log viewer with level filtering
  - **Command Line**: terminal-style command input/output
- **Progress overlay**: shows operation progress above the panel

All C++ backend interfaces are **mocked in pure QML** (ListModel + JS). No C++ classes are created in this phase. The mock layer is designed so a future C++ backend can replace it by swapping the model source.

Colors and styling are **fully derived from AppTheme** — no hardcoded terminal color values.

Icons are sourced from **ionicons 8.0 outline** variant.

## 2. Architecture — 5 New QML Files

```
src/app/resource/qml/
├── sections/
│   └── ActivityOverlay.qml       ← Container + Activity button + Progress bar
├── components/
│   ├── ActivityPanel.qml         ← Main panel shell (tab bar + view switching)
│   ├── LogEventsView.qml         ← Events tab content (ListView + filters)
│   ├── LogLevelChip.qml          ← Reusable level filter chip
│   └── TerminalView.qml          ← Command Line tab content
```

### Integration Point (Main.qml)

ActivityOverlay is placed **inside the `Item` that wraps the content `RowLayout`** (the one containing SidebarPanel + ViewportPanel), using absolute positioning with `z: 40` to float above the viewport.

```qml
// Main.qml — inside the Item wrapping the content RowLayout
ActivityOverlay {
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    anchors.rightMargin: appTheme.gap
    anchors.bottomMargin: appTheme.gap
    theme: appTheme
    z: 40
}
```

## 3. Component Specifications

### 3.1 ActivityOverlay.qml (sections/)

**Responsibilities:**
- Container for the Activity button and panel
- Calculates available width from parent
- Manages open/close state
- Contains inline progress bar (simplified from reference's OperationProgressOverlay)

**Properties:**
```
required property AppTheme theme
property bool activityOpen: false
property bool hasNewErrors: false        // drives red badge
property bool hasNewLogs: false          // drives orange badge
property real progress: -1               // -1 = hidden, 0..1 = determinate
property string progressStatus: ""       // "Running" / "Done" / "Failed"
```

**Layout:**
```
┌──────────────────────────────────────────┐
│ [Progress bar — full width, 6px tall]    │  ← only visible when progress >= 0
│                                          │
│ ┌──────────────────────────────────────┐ │
│ │         ActivityPanel                │ │  ← only visible when activityOpen
│ └──────────────────────────────────────┘ │
│                                          │
│              [⏺ Activity] button         │  ← always visible, bottom-right
└──────────────────────────────────────────┘
```

**Width:** `Math.min(920, parent.width * 0.5)`

**Activity button:**
- Icon: `pulse` (from ionicons)
- Text: "Activity"
- Notification dot: pulsing red (errors) or orange (logs), 800ms loop
- Height: 38px
- Click toggles `activityOpen`

**Progress bar (inline):**
- 6px tall rectangle, full panel width
- States: active (animated indeterminate), success (green), error (red), idle (hidden)
- Auto-hides after 3s (success) or 6s (error)
- Colors: accentA (running), success (done), danger (failed)
- Mock phase note: `progress` stays at -1 (no mock trigger), so progress bar is invisible until a future C++ backend drives it

### 3.2 ActivityPanel.qml (components/)

**Responsibilities:**
- Tab bar: Events | Command Line
- Hosts LogEventsView and TerminalView
- Manages mock data models
- Passes theme and model references to child views

**Properties:**
```
required property AppTheme theme
property int currentTab: 0              // 0 = Events, 1 = Command Line
```

**Internal mock models:**

```qml
// Mock log entries
ListModel {
    id: mockLogModel
    // Pre-populated with ~8 sample entries of varying levels
    // Component.onCompleted populates via appendMockEntry()
}

// Mock terminal entries
ListModel {
    id: mockTerminalModel
    // Starts empty; entries added when user submits commands
}
```

**Mock helper functions:**
```js
function appendMockEntry() {
    // Adds a sample log entry with random level, source, message, timestamp
}

function appendTerminalEntry(type, text) {
    // type: "command" | "response" | "error"
    // Caps at 160 entries
}

function runCommand(text) {
    // Mock: echoes command, produces fake JSON response after 200ms delay
    appendTerminalEntry("command", text)
    delayTimer.start()  // simulates async response
}
```

**Layout:**
```
┌─────────────────────────────────────────────────┐
│ Activity Center                          [✕]    │  ← header with close button
├─────────────────────────────────────────────────┤
│ [📋 Events] [💻 Command Line]                   │  ← tab bar
├─────────────────────────────────────────────────┤
│                                                   │
│   <LogEventsView>  or  <TerminalView>            │  ← tab content
│                                                   │
└─────────────────────────────────────────────────┘
```

**Panel sizing:**
- Width: fills parent (ActivityOverlay width)
- Height: 400px (Events), 500px (Command Line) — adjustable
- Background: theme.surface with 0.96 opacity (dark) / 0.98 (light)
- Border: theme.borderSubtle
- Radius: theme.radiusLarge (24px)
- Shadow: subtle drop shadow via slightly larger background rectangle

**Show/hide animation:**
- Opacity: 0→1 (180ms, Easing.OutQuad)
- Y offset: +10→0 (180ms, Easing.OutQuad)

### 3.3 LogEventsView.qml (components/)

**Responsibilities:**
- Displays log entries in a scrollable ListView
- Provides a filter control bar
- Contains inline delegate for log entry cards

**Properties:**
```
required property AppTheme theme
required property var model             // ListModel from ActivityPanel
property int enabledLevelMask: 0x3F    // all 6 levels enabled (bits 0-5)
```

**Filter bar:**
```
┌─────────────────────────────────────────────────────────┐
│ [Trace] [Debug] [Info] [Warn] [Error] [Critical] [🗑]  │
└─────────────────────────────────────────────────────────┘
```

- 6 × LogLevelChip (toggleable, each controls one bit in enabledLevelMask)
- Filter toggle button (funnel icon) — expands/collapses the chip row
- Collapse indicator (chevronDown icon) — rotates when filter row is expanded
- Clear button (trash icon) — clears all entries from model
- Filter logic: JS function `levelVisible(level)` checks bitmask

**Log entry delegate (inline Component):**
```
┌──────────────────────────────────────────────────┐
│ ║ [LEVEL] SourceName              12:34:56       │
│ ║ Log message text that can wrap to multiple     │
│ ║ lines if needed                                │
│ ║ tid 1234 · file.cpp:256                        │
└──────────────────────────────────────────────────┘
```

- Left accent bar: 4px wide, colored by level
- Level chip: small inline badge (accentA/B/C/D)
- Source name: bold, ellided
- Time: monospace, right-aligned
- Message: wrapping text
- Metadata: thread ID + file:line (optional, shown if present)

**Level → Color mapping (from AppTheme):**
| Level | Name     | Color    |
|-------|----------|----------|
| 0     | TRACE    | accentA  |
| 1     | DEBUG    | accentA  |
| 2     | INFO     | accentB  |
| 3     | WARN     | accentC  |
| 4     | ERROR    | accentD  |
| 5     | CRITICAL | accentD  |

**Auto-scroll:** When new entries are added while scrolled to bottom, auto-scroll to show latest entry.

### 3.4 LogLevelChip.qml (components/)

**Responsibilities:**
- Interactive toggle chip for a single log level
- Visual feedback for selected/unselected state

**Properties:**
```
required property AppTheme theme
property string text: ""
property color accentColor: theme.accentA
property bool selected: false
signal clicked
```

**Sizing:** auto-width based on text + 22px padding, height 28px

**Appearance:**
- Selected: accent tint background, accent border, bold text
- Unselected: muted surface background, subtle border, secondary text
- Hover: pointer cursor

### 3.5 TerminalView.qml (components/)

**Responsibilities:**
- Displays command/response history
- Provides text input area for commands
- Mock command execution (echo + fake response)

**Properties:**
```
required property AppTheme theme
required property var model             // ListModel from ActivityPanel
signal commandSubmitted(string text)    // emitted on Ctrl+Enter
```

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│ [Terminal output — scrollable]                       │
│                                                       │
│ >>> process({"action":"describe"})    ← command      │
│ <<< {"status":"ok","result":{...}}   ← response     │
│ >>> invalid syntax                                   │
│ !!! SyntaxError: unexpected token    ← error         │
│                                                       │
├─────────────────────────────────────────────────────┤
│ > █                                    [▶ Run]       │
│ (Type command, Ctrl+Enter to run)                    │
└─────────────────────────────────────────────────────┘
```

**Terminal entry types and colors (derived from AppTheme):**
| Type     | Prefix | Color              |
|----------|--------|--------------------|
| command  | `>>>`  | theme.success      |
| response | `<<<`  | theme.warning      |
| error    | `!!!`  | theme.danger       |

**Input area:**
- TextEdit with WrapAnywhere
- Min height: 58px, max height: 156px (grows with content)
- Placeholder: "Type a command..." (textTertiary)
- Ctrl+Enter: emits `commandSubmitted`, clears input
- Run button (play icon): same as Ctrl+Enter

**Terminal entry limit:** 160 entries (oldest removed when exceeded)

**Scrollbar:** 6px wide, custom themed (surfaceStrong thumb on borderSubtle track)

## 4. Icons — Ionicons Outline

Copy from `D:\WorkSpace\OGLWorkSpace\ionicons-8.0.13\src\svg\` to `src/app/resource/icons/`:

| Source (ionicons)                | Target filename     | Usage                        |
|----------------------------------|---------------------|------------------------------|
| pulse-outline.svg                | pulse.svg           | Activity button icon         |
| list-outline.svg                 | list.svg            | Events tab icon              |
| terminal-outline.svg             | terminal.svg        | Command Line tab icon        |
| close-outline.svg                | closePanel.svg      | Panel close button           |
| trash-outline.svg                | trash.svg           | Clear logs/terminal          |
| funnel-outline.svg               | funnel.svg          | Level filter toggle (LogEventsView filter bar) |
| chevron-down-outline.svg         | chevronDown.svg     | Expand/collapse filter panel (LogEventsView)   |
| play-outline.svg                 | play.svg            | Run command button           |

Note: `close.svg` would conflict with potential existing names; use `closePanel.svg`.

Register all 8 new icons in `src/app/CMakeLists.txt` RESOURCES section.

## 5. Mock Data Strategy

### Sample Log Entries (pre-populated)

```js
const sampleEntries = [
    { level: 4, levelName: "ERROR",  source: "GeometryKernel", message: "Boolean operation failed: self-intersecting input", time: "14:32:07", threadId: 1024, file: "boolean_op.cpp", line: 342 },
    { level: 2, levelName: "INFO",   source: "SceneManager",   message: "Scene loaded successfully (12 objects)", time: "14:32:05", threadId: 1, file: "", line: 0 },
    { level: 3, levelName: "WARN",   source: "MeshGenerator",  message: "Degenerate triangle detected, skipping face #847", time: "14:32:04", threadId: 2048, file: "mesh_gen.cpp", line: 156 },
    { level: 1, levelName: "DEBUG",  source: "RenderPipeline", message: "Frame buffer resized to 1920x1080", time: "14:32:03", threadId: 1, file: "", line: 0 },
    { level: 2, levelName: "INFO",   source: "PluginLoader",   message: "Loaded 3 plugins: geometry, mesh, export", time: "14:32:01", threadId: 1, file: "", line: 0 },
    { level: 0, levelName: "TRACE",  source: "EventLoop",      message: "Processing 42 pending events", time: "14:31:58", threadId: 1, file: "event_loop.cpp", line: 89 },
    { level: 5, levelName: "CRITICAL", source: "MemoryPool", message: "Allocation failed: out of memory (requested 2.1 GB)", time: "14:31:55", threadId: 4096, file: "memory_pool.cpp", line: 67 },
    { level: 2, levelName: "INFO",   source: "CommandRecorder", message: "Recording started", time: "14:31:50", threadId: 1, file: "", line: 0 },
];
```

### Mock Terminal Behavior

When user submits a command:
1. `appendTerminalEntry("command", text)` — shows in green
2. After 200ms delay:
   - If text starts with `{`: try JSON.parse, show formatted response
   - Otherwise: show generic "Command executed" response
   - On parse error: show error entry

## 6. AppTheme Additions

No new theme properties needed. The panel derives all colors from existing properties:

- **Panel background:** `theme.tint(theme.surface, darkMode ? 0.96 : 0.98)`
- **Tab selected bg:** `theme.tint(theme.accentA, darkMode ? 0.2 : 0.1)`
- **Tab border:** `theme.borderSubtle`
- **Log card bg:** `theme.tint(theme.surface, darkMode ? 0.72 : 0.98)`
- **Log card border:** `theme.tint(accentColor, darkMode ? 0.36 : 0.2)`
- **Terminal bg:** `theme.tint(theme.surface, darkMode ? 0.5 : 1.0)`
- **Terminal input bg:** `theme.surfaceMuted`

## 7. Animations

| Animation            | Duration | Easing        | Properties              |
|----------------------|----------|---------------|-------------------------|
| Panel show/hide      | 180ms    | OutQuad       | opacity (0↔1), y (+10↔0) |
| Notification badge   | 800ms    | Loop          | opacity (1.0↔0.38)      |
| Progress bar         | 1200ms   | Linear loop   | x position (indeterminate) |
| Progress auto-hide   | 3000ms   | —             | Timer-based              |

## 8. Translation Keys

All user-visible strings must use `qsTr()`. New keys:

- `"Activity"` — button label
- `"Activity Center"` — panel title
- `"Events"` — tab label
- `"Command Line"` — tab label
- `"Type a command..."` — input placeholder
- `"TRACE"`, `"DEBUG"`, `"INFO"`, `"WARN"`, `"ERROR"`, `"CRITICAL"` — level names
- `"Clear"` — clear action tooltip
- `"Run"` — run button tooltip
- `"tid %1"` — thread ID format

## 9. Constraints & Non-Goals

**In scope:**
- All 5 QML components with full visual design
- Mock data with realistic sample entries
- Dark/light theme support
- Animations (show/hide, badge pulse, progress)
- Icon integration from ionicons
- Translation-ready strings
- Update `opengeolab_zh_CN.ts` with Chinese translations for new strings

**Out of scope (deferred to future):**
- C++ OperationLogService / OperationLogModel
- Real spdlog integration
- Real Python/JSON command execution (appController)
- CodePanel / OutputStrip (service response visualization)
- Keyboard shortcuts beyond Ctrl+Enter
- Persistent log storage
- Export/save log functionality

## 10. Future C++ Integration Points

When real C++ backend is ready, the following changes are needed:

1. **Replace mock ListModel** in ActivityPanel with `Q_PROPERTY(QAbstractItemModel* model)` from OperationLogService
2. **Replace mock runCommand()** with `appController.submitServiceRequest()` / `appController.runEmbeddedPythonCommandLine()`
3. **Replace mock progress** with real operation progress signals
4. **Replace mock hasNewErrors/hasNewLogs** with OperationLogService notifications

The QML component interfaces are designed to make this swap minimal — only ActivityPanel's model source changes; LogEventsView, TerminalView, and LogLevelChip remain untouched.
