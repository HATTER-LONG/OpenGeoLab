# QML Action System & Style Refactoring — Design Spec

**Date:** 2026-03-22
**Branch:** `dev/v2-dev-03-22`
**Scope:** 8 QML files (modify 7, create 1) + 1 CMakeLists.txt

---

## 1. Problem Statement

The current QML codebase has four tightly coupled issues:

1. **Action definitions scattered** — RibbonConfig defines ribbon actions centrally, but HeaderMenuPanel hardcodes 8 ActionButton instances with individual color/icon/callback assignments.
2. **Signal chain too long** — Ribbon actions traverse 4 layers of signals (RibbonTile → HeaderRibbonGroup → AppHeader → Main) before reaching the handler.
3. **`tint()` calls inlined everywhere** — 24+ identical `theme.tint(theme.X, darkMode ? A : B)` expressions scattered across RibbonTile, ActionButton, AppHeader, HeaderMenuPanel.
4. **Style definitions unsystematic** — Each button component exposes 6–8 color properties; callers must set them individually with no reuse.

These issues make it expensive to add a new action (touch 3+ files, copy-paste color boilerplate) and fragile to change visual style (find-and-replace across many files).

---

## 2. Design Decisions (Confirmed)

| Decision | Choice |
|----------|--------|
| Config unification | Keep RibbonConfig + **new MenuConfig** (separate but structurally consistent) |
| Style system | Pre-computed color sets in AppTheme (QtObject groups + parameterized function) |
| Signal chain | Callback-passing via `required property` — eliminate intermediate signals |
| Interface changes | Allowed — visual appearance must remain pixel-identical |

---

## 3. Architecture

### 3.1 Data Flow (After)

```
Main.qml
 ├── openActionPage(key)  ──(property)──▶  AppHeader.actionHandler
 │       ├── HeaderRibbonGroup  ──(property)──▶  RibbonTile.actionHandler(key)
 │       └── HeaderMenuPanel    ──(property)──▶  ActionButton.actionHandler(key)
 └── RibbonConfig (unchanged)
 └── MenuConfig (new)
```

**Key change:** `signal triggerAction` removed from HeaderRibbonGroup, AppHeader, HeaderMenuPanel. `signal requestThemeToggle` removed from AppHeader, HeaderMenuPanel (toggleTheme now routed via actionHandler). Replaced by a function reference passed down via `required property var actionHandler`.

### 3.2 Color Flow (After)

```
AppTheme
 ├── ribbonTile: QtObject { normal, hovered, pressed, borderNormal, iconBg }
 ├── panel: QtObject { normal, border, tabBarBorder, tabActiveBg, tabActiveBorder,
 │                      tabHovered, menuBg, menuBorder, menuRecorderBg, separator }
 └── actionButtonColors(accentName, alphaScale): { normal, pressed }
 └── accentHoverBorder(accentName): color  (only for buttons with hoverAccent)
```

- Components reference pre-computed colors: `theme.ribbonTile.normal` instead of `theme.tint(theme.surface, theme.darkMode ? 0.3 : 0.66)`.
- Accent-dependent colors in RibbonTile (border hover, icon border, gradient) remain inline `tint()` with `accentOne`/`accentTwo` — these are per-button parameterized and cannot be pre-computed.
- Hamburger menu button in AppHeader has 3 inline `tint()` calls for its unique hover/pressed/border states — kept inline (single instance, not worth a color set).

---

## 4. Component Specifications

### 4.1 MenuConfig.qml (New)

**Location:** `src/app/resource/qml/MenuConfig.qml`
**Role:** Central data source for menu panel actions, parallel to RibbonConfig.

```
QtObject {
    readonly property var sections: [
        {
            "title": qsTr("Workspace"),
            "accent": "accentA",
            "actions": [
                { "key": "importModel",  "title": qsTr("Import Model"),  "icon": "import"  },
                { "key": "exportModel",  "title": qsTr("Export Model"),  "icon": "export"  },
                { "key": "toggleTheme",  "title": "",  "icon": "",  "dynamic": true, "alphaScale": "muted", "hoverAccent": "accentA" },
                { "key": "switchLanguage", "title": "", "icon": "language", "dynamic": true,
                  "accent": "accentE", "alphaScale": "muted", "hoverAccent": "accentE" }
            ]
        },
        {
            "title": qsTr("Script Recorder"),
            "accent": "accentB",
            "actions": [
                { "key": "recordSelection",       "title": qsTr("Start Script Record"),  "icon": "record"       },
                { "key": "replayCommands",         "title": qsTr("Replay Script"),        "icon": "replay"       },
                { "key": "exportScript",           "title": qsTr("Export Record"),         "icon": "exportRecord" },
                { "key": "clearRecordedCommands",  "title": qsTr("Clear Script History"), "icon": "clear",
                  "hoverAccent": "accentD" }
            ]
        }
    ]
}
```

**Notes:**
- `dynamic: true` actions have their `title` and `icon` resolved at render time via binding expressions in HeaderMenuPanel.
- `accent` at section level is the default; an action-level `accent` overrides it (e.g., switchLanguage uses `accentE`).
- `alphaScale: "muted"` reduces normal/pressed alphas to match current toggleTheme/switchLanguage (0.18/0.1 and 0.28/0.16 instead of 0.2/0.11 and 0.3/0.18).
- `hoverAccent` overrides the hover border accent (e.g., clearRecordedCommands uses `accentD` for danger semantics).

### 4.2 AppTheme.qml (Extended)

Add the following after the existing `accentByName()` function:

```
// ── Pre-computed color sets ──────────────────────────────────────

readonly property QtObject ribbonTile: QtObject {
    readonly property color normal: root.tint(root.surface, root.darkMode ? 0.3 : 0.66)
    readonly property color hovered: root.tint(root.surfaceMuted, root.darkMode ? 0.9 : 0.96)
    readonly property color pressed: root.tint(root.surfaceStrong, root.darkMode ? 0.94 : 0.98)
    readonly property color borderNormal: root.tint(root.borderSubtle, root.darkMode ? 0.88 : 0.72)
    readonly property color iconBg: root.tint(root.surface, root.darkMode ? 0.82 : 0.95)
}

readonly property QtObject panel: QtObject {
    readonly property color normal: root.tint(root.surface, root.darkMode ? 0.7 : 1.0)
    readonly property color border: root.tint(root.borderSubtle, 0.7)
    readonly property color tabBarBorder: root.tint(root.borderSubtle, 0.78)
    readonly property color tabActiveBg: root.tint(root.accentA, root.darkMode ? 0.2 : 0.12)
    readonly property color tabActiveBorder: root.tint(root.accentA, root.darkMode ? 0.42 : 0.24)
    readonly property color tabHovered: root.tint(root.surfaceStrong, root.darkMode ? 0.54 : 0.74)
    readonly property color menuBg: root.tint(root.surfaceMuted, root.darkMode ? 0.5 : 0.74)
    readonly property color menuBorder: root.tint(root.borderSubtle, 0.7)
    readonly property color menuRecorderBg: root.tint(root.surfaceMuted, root.darkMode ? 0.46 : 0.72)
    readonly property color separator: root.tint(root.borderSubtle, 0.6)
}

/// Return a color set for ActionButton based on accent name and alpha scale.
/// @param accentName  AppTheme accent name (e.g. "accentA", "accentE")
/// @param alphaScale  "normal" (default) or "muted" (dimmer for tool buttons)
function actionButtonColors(accentName: string, alphaScale: string): var {
    const accent = root.accentByName(accentName);
    if (alphaScale === "muted") {
        return {
            normal:      root.tint(accent, root.darkMode ? 0.18 : 0.1),
            pressed:     root.tint(accent, root.darkMode ? 0.28 : 0.16)
        };
    }
    return {
        normal:      root.tint(accent, root.darkMode ? 0.2  : 0.11),
        pressed:     root.tint(accent, root.darkMode ? 0.3  : 0.18)
    };
}

/// Return the hover border color for a specific accent.
/// Only used by buttons that explicitly specify hoverAccent in MenuConfig.
function accentHoverBorder(accentName: string): color {
    const accent = root.accentByName(accentName);
    return root.tint(accent, root.darkMode ? 0.58 : 0.34);
}
```

**Values are exact copies from the current hardcoded expressions** — no visual change.

**Notes on inline tint() that remain:**
- RibbonTile: ~6 accent-dependent tint() calls (gradients, icon border, border hover) — parameterized by accentOne/accentTwo, cannot pre-compute.
- AppHeader hamburger button: 3 tint() calls (pressed/hovered/border) — single instance, not worth a color set.
- ActionButton hover overlay: **removed** (see Section 4.4).

### 4.3 RibbonTile.qml (Refactored)

**Interface changes:**

| Before | After |
|--------|-------|
| `signal clicked` | `required property var actionHandler` |
| (none) | `property string actionKey: ""` |
| `property color accentOne: theme.accentA` | (kept) |
| `property color accentTwo: theme.accentB` | (kept) |
| Inline `theme.tint(...)` × 9 | 3 accent-independent → `theme.ribbonTile.*`; 6 accent-dependent stay inline |

**Key behavior:**
- MouseArea `onClicked` calls `tile.actionHandler(tile.actionKey)` instead of emitting signal
- `color:` expression becomes:
  ```
  mouseArea.pressed ? theme.ribbonTile.pressed
      : (mouseArea.containsMouse ? theme.ribbonTile.hovered : theme.ribbonTile.normal)
  ```
- `border.color` (non-hover): `theme.ribbonTile.borderNormal`
- Icon background: `theme.ribbonTile.iconBg`
- Accent-dependent colors (border hover, icon border, gradient stops) remain inline `tint()` with `accentOne`/`accentTwo`.

### 4.4 ActionButton.qml (Simplified)

**Interface changes:**

| Before | After |
|--------|-------|
| `property color buttonColor` | `property var colorSet: ({})` |
| `property color pressedColor` | (removed — in colorSet) |
| `property color hoverBorderColor` | (removed — in colorSet, optional) |
| `property color iconPrimaryColor` | (kept) |
| `property color iconSecondaryColor` | **removed** |
| `property color labelColor` | (kept) |
| `signal clicked` | **removed** |
| (none) | `required property var actionHandler` |
| (none) | `property string actionKey: ""` |
| `property bool quiet` | (kept — affects border idle state) |

**Hover overlay rectangle (lines 31–36): removed.** The current overlay uses `iconSecondaryColor` at 6–8% alpha, which is near-invisible. Removing it simplifies the component without visible impact.

**Color mapping:**
```
color: mouseArea.pressed ? colorSet.pressed
    : (mouseArea.containsMouse ? theme.tint(colorSet.normal, quiet ? 0.92 : 1.0)
    : colorSet.normal)
border.color: mouseArea.containsMouse
    ? (hoverBorderOverride.a > 0 ? hoverBorderOverride
       : theme.tint(theme.textPrimary, theme.darkMode ? 0.52 : 0.3))
    : (quiet ? theme.tint(theme.borderSubtle, 0.45) : theme.borderSubtle)
```

**Key details:**
- New `property color hoverBorderOverride: "transparent"` — set by caller when action has `hoverAccent`. Default is transparent (alpha 0), which triggers the neutral gray fallback via the `a > 0` check. This preserves the current behavior for the 5 buttons that don't set a custom hover border.
- `quiet` border behavior is preserved for potential future use.
- MouseArea `onClicked` calls `control.actionHandler(control.actionKey)`.
- `colorSet` now only contains `{ normal, pressed }` — no `hoverBorder`.

### 4.5 HeaderMenuPanel.qml (Rewritten)

**Before:** 198 lines, 8 hardcoded ActionButton instances.
**After:** ~90 lines, data-driven from MenuConfig.

Structure:
```
Rectangle (panel container)
 └── Column
      └── Repeater { model: menuConfig.sections }
           └── Column
                ├── Text (section title)
                ├── Rectangle (section container, bg from panel.menuBg / panel.menuRecorderBg)
                │    └── Column
                │         └── Repeater { model: section.actions }
                │              └── ActionButton { colorSet, actionHandler, leftAligned: true, ... }
                └── Rectangle (separator — between sections only, color: theme.panel.separator)
```

**Color set and hover border resolution per action (declarative):**
```qml
Components.ActionButton {
    required property var modelData
    // Resolve per-action accent, falling back to section accent
    readonly property string effectiveAccent: modelData.accent ?? sectionAccent
    readonly property string effectiveAlpha: modelData.alphaScale ?? "normal"
    colorSet: theme.actionButtonColors(effectiveAccent, effectiveAlpha)
    // hoverBorderOverride is reactive (re-evaluates on darkMode change)
    hoverBorderOverride: modelData.hoverAccent
        ? theme.accentHoverBorder(modelData.hoverAccent) : "transparent"
}
```

**Dynamic button handling:**
```qml
Components.ActionButton {
    required property var modelData
    leftAligned: true
    buttonText: {
        if (modelData.key === "toggleTheme")
            return panel.darkMode ? qsTr("Switch to Light") : qsTr("Switch to Dark");
        if (modelData.key === "switchLanguage")
            return TranslationManager.currentLanguage === "zh_CN"
                ? qsTr("Switch to English") : qsTr("Switch to Chinese");
        return modelData.title;
    }
    iconKind: {
        if (modelData.key === "toggleTheme")
            return panel.darkMode ? "lightTheme" : "darkTheme";
        return modelData.icon;
    }
}
```

**Special action handling (toggleTheme/switchLanguage):**
- All buttons call `panel.actionHandler(key)` uniformly
- `switchLanguage` needs `TranslationManager.switchLanguage()` called **before** the action handler fires the status update. This is handled in Main.openActionPage (see §4.8), not in the delegate.
- `requestThemeToggle` signal is **removed** — Main.openActionPage already handles "toggleTheme"

### 4.6 HeaderRibbonGroup.qml (Simplified)

**Before:** Receives actions, renders RibbonTiles, emits `signal triggerAction`.
**After:** Receives `required property var actionHandler`, passes it directly to each RibbonTile.

```qml
// Before
signal triggerAction(string actionKey)
// RibbonTile { onClicked: root.triggerAction(actionData.key) }

// After
required property var actionHandler
// RibbonTile { actionKey: actionData.key; actionHandler: root.actionHandler }
```

### 4.7 AppHeader.qml (Simplified)

**Signals removed:** `triggerAction`, `requestThemeToggle`
**Signals kept:** `toggleMenu`, `selectTab` — these are UI-specific, not action dispatch.

**New property:**
```qml
required property var actionHandler  // function(string key)
```

Passed to:
- `HeaderRibbonGroup { actionHandler: header.actionHandler }`
- `HeaderMenuPanel { actionHandler: header.actionHandler }`

### 4.8 Main.qml (Minor Changes)

```qml
AppHeader {
    // ...
    actionHandler: root.openActionPage  // function reference, no wrapper needed
    onToggleMenu: root.menuOpen = !root.menuOpen
    onSelectTab: function(tabIndex) { root.selectedRibbonTab = tabIndex }
}
```

`openActionPage` handles all actions uniformly (unchanged logic):
```qml
function openActionPage(actionKey) {
    if (actionKey === "toggleTheme") {
        root.toggleTheme();
        return;
    }
    if (actionKey === "switchLanguage") {
        // Perform the actual language switch here (moved from HeaderMenuPanel)
        TranslationManager.switchLanguage(
            TranslationManager.currentLanguage === "zh_CN" ? "en_US" : "zh_CN");
        root.statusNote = TranslationManager.currentLanguage === "zh_CN"
            ? qsTr("Switched to Chinese.") : qsTr("Switched to English.");
        root.menuOpen = false;
        return;
    }
    root.statusNote = qsTr("Action: %1").arg(actionKey);
    root.menuOpen = false;
    console.log("[Main] openActionPage:", actionKey);
}
```

**Note:** `TranslationManager.switchLanguage()` is moved from HeaderMenuPanel delegate into Main.openActionPage. This centralizes all side effects in one place, consistent with the "all actions route through Main" design.
**Note:** `onRequestThemeToggle` handler removed from AppHeader instantiation — toggleTheme now routes via actionHandler.

---

## 5. Edge Cases

| Case | Handling |
|------|----------|
| Dynamic button titles after language switch | qsTr() in binding expressions re-evaluates automatically |
| MenuConfig qsTr() in `readonly property var` | Property binding recalculates on language change |
| `actionButtonColors()` called per render | Only 2 sections × rerenders; negligible cost |
| ActionButton used outside HeaderMenuPanel | Not currently the case; if needed later, provide default empty `colorSet` |
| Adding a new menu action | Add entry to MenuConfig.sections — single file change |
| Adding a new ribbon action | Add to RibbonConfig.groupsModel — same as before, single file |
| Per-action accent override | Action-level `accent` overrides section default; `hoverAccent` overrides hover border |
| Buttons without explicit hoverBorder | `hoverBorderOverride` defaults to transparent; ActionButton falls back to neutral gray |
| `quiet` mode on ActionButton | Kept for future use; affects idle border (0.45 alpha vs full) |

---

## 6. Files Changed Summary

| File | Action | Lines (est.) |
|------|--------|-------------|
| `MenuConfig.qml` | Create | ~50 |
| `AppTheme.qml` | Extend | +40 |
| `RibbonTile.qml` | Refactor | ~137 → ~120 |
| `ActionButton.qml` | Simplify | ~83 → ~65 |
| `HeaderMenuPanel.qml` | Rewrite | ~198 → ~90 |
| `HeaderRibbonGroup.qml` | Simplify | ~67 → ~55 |
| `AppHeader.qml` | Simplify | ~185 → ~170 |
| `Main.qml` | Minor | ~163 → ~155 |
| `CMakeLists.txt` | Add MenuConfig.qml | +1 line |

**Net effect:** ~833 lines → ~705 lines (−128 lines, −15%)

---

## 7. Verification Plan

1. **Build:** `cmake --build build --config RelWithDebInfo --parallel 4`
2. **Visual regression (manual):**
   - Ribbon tabs switch correctly, all buttons render with correct accent gradients
   - Menu panel opens/closes with animation, all 8 actions fire correctly
   - toggleTheme button shows correct dimmer tint (muted alpha) and toggles dark/light
   - switchLanguage button shows purple tint (accentE) and switches language
   - clearRecordedCommands hover border shows orange/red (accentD danger)
   - Import/Export/Record/Replay/ExportRecord hover borders show neutral gray
   - Activity panel and overlay unaffected
3. **Action dispatch test:**
   - Click every ribbon button → console shows `[Main] openActionPage: <key>`
   - Click every menu button → same
4. **Regression:** No changes to SidebarPanel, ViewportPanel, ActivityPanel, or C++ backend

---

## 8. Out of Scope

- No changes to ActivityPanel, LogEventsView, TerminalView, SidebarPanel, ViewportPanel
- No changes to C++ backend (TranslationManager, etc.)
- No new automated tests (project has no QML test infrastructure)
- No visual redesign — appearance is preserved exactly
