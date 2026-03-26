# QML UI Cleanup — Removed Features Log

This document records all features and service dependencies removed from the
QML UI during the initial cleanup. Each item should be re-implemented once the
corresponding C++ / Python backend service is available.

---

## 1. `OpenGeoLab.Services` — Module Removed

The following C++ singleton services were referenced by QML but have **no
implementation yet**. All `import OpenGeoLab.Services` statements have been
removed.

### 1.1 `RequestService`

| File | Usage | Status |
|------|-------|--------|
| `Main.qml` | `RequestService.submitAsync(...)` — geometry create, plugin list, box list | Removed |
| `Main.qml` | `RequestService.executeOnMainThread(...)` — plugin UI invocation | Removed |
| `Main.qml` | `Connections { target: RequestService }` — `onResponseReady`, `onErrorOccurred` signals | Removed |

**To restore:** Implement a `RequestService` QObject singleton exposing
`Q_INVOKABLE submitAsync(QString json)`, `Q_INVOKABLE executeOnMainThread(QString json)`,
and signals `responseReady(QString requestId, QString responseJson)`,
`errorOccurred(QString requestId, QString errorMessage)`. Register it as a QML
singleton in the `OpenGeoLab.Services` module.

### 1.2 `NotificationService`

| File | Usage | Status |
|------|-------|--------|
| `Main.qml` | `Connections { target: NotificationService }` — `onNotificationReceived(channel, payload)` | Removed |

**To restore:** Implement a `NotificationService` QObject singleton with signal
`notificationReceived(QString channel, QString payload)`.

### 1.3 `ProgressTracker`

| File | Usage | Status |
|------|-------|--------|
| `ProgressCard.qml` | `ProgressTracker.hasActiveTasks`, `.currentProgress`, `.statusText`, `.currentMessage` | Replaced with static defaults (`false`, `0`, `""`, `""`) |
| `ProgressCard.qml` | `Connections { target: ProgressTracker }` — `onTaskStarted`, `onTaskCompleted` | Removed |

**To restore:** Implement a `ProgressTracker` QObject singleton with properties
`hasActiveTasks: bool`, `currentProgress: real`, `statusText: string`,
`currentMessage: string` and signals `taskStarted(QString taskId)`,
`taskCompleted(QString taskId, bool success)`.

### 1.4 `TranslationManager` — RESTORED

`TranslationManager` has been re-implemented as a C++ QML singleton in the
`OpenGeoLab.App` module. Language switching between `en_US` and `zh_CN` is
fully functional.

| File | Status |
|------|--------|
| `include/opengeolab/app/translation_manager.h` | Implemented |
| `src/translation_manager.cpp` | Implemented |

---

## 2. `Qt5Compat.GraphicalEffects` — RESTORED

| File | Usage | Status |
|------|-------|--------|
| `AppIcon.qml` | `ColorOverlay` for theme-aware icon tinting | Restored; linked against `Qt6::Core5Compat` |

---

## 3. Action Dispatch — Simplified

`Main.qml :: openActionPage(actionKey)` previously dispatched actions to backend
services. The following action keys are now **stub-only** (log to console):

| Action Key | Original Behavior | Current Behavior |
|------------|-------------------|------------------|
| `switchLanguage` | Called `TranslationManager.switchLanguage(...)` | **Restored** — fully functional |
| `addBox` | `RequestService.submitAsync(...)` create_box | Logs to console |
| `addCylinder`, `addSphere`, `addTorus` | Not yet implemented | Logs to console |
| `trim`, `offset`, `queryGeometry` | Not yet implemented | Logs to console |
| `generateMesh`, `smoothMesh`, `queryMesh` | Not yet implemented | Logs to console |
| `aiSuggest`, `aiChat` | Not yet implemented | Logs to console |
| `pluginUI_*` | `RequestService.executeOnMainThread(...)` | Logs to console |
| `plugin_*` | `RequestService.submitAsync(...)` | Logs to console |
| `importModel`, `exportModel` | Not yet implemented | Logs to console |
| `recordSelection`, `replayCommands`, `exportScript`, `clearRecordedCommands` | Not yet implemented | Logs to console |

**`toggleTheme`** remains fully functional (dark/light mode switch).

---

## 4. Data Population — Removed

| Feature | Original Source | Current State |
|---------|----------------|---------------|
| Plugin list in Plugins ribbon tab | `RequestService` → `Component.onCompleted` fetch | Empty; shows "No plugins found" |
| Box list in Scene sidebar | `NotificationService` → geometry data_changed → refresh | Empty; shows "No geometry yet" |
| Progress card | `ProgressTracker` properties | Always hidden (`active: false`) |

---

## 5. QML Module Registration Fixes

| File | Change |
|------|--------|
| `theme/qmldir` | Removed `singleton` keyword — `AppTheme` is instantiated per-window |
| `sections/qmldir` | Added `ActivityOverlay 1.0 ActivityOverlay.qml` |
| `components/qmldir` | Added `ActivityPanel`, `PluginRibbonGroup`, `LogEventsView`, `LogLevelChip`, `StatChip`, `TerminalView` |
| `qml/qmldir` (new) | Created root qmldir exposing `MenuConfig` and `RibbonConfig` for sub-directory imports |
