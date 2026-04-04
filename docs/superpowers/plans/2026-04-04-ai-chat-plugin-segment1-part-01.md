# AI Chat Plugin Segment 1 — Part 1 of 2

> Part 文件：包含 Tasks 1–4（CMake 依赖、共享主题、插件骨架 + scene_tools、JSON 高亮器）。

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **Recommended models:** Use Claude Opus 4.6 for design or plan revisions. Use GPT-5.4 for implementation, debugging, verification, and code-review subtasks.

**Goal:** Build the foundation of the AI Chat Plugin — CMake dependency management, shared theme module, plugin skeleton with cached schema API, and a reusable JSON syntax highlighter.

**Architecture:** Python plugin (`plugins/ai_chat_plugin/`) following the existing plugin pattern (`describe_plugin()` + `launch_ui()`). Scene tools cache the full `opengeolab_pywrapper.describe()` schema and expose a 4-layer progressive discovery API. A shared `_shared` plugin package extracts `plugin_theme.py` from `demo_ui_plugin` for cross-plugin reuse.

**Tech Stack:** Python 3.13, PySide6 6.9, CMake, pybind11 (opengeolab_pywrapper)

**Spec:** `docs/superpowers/specs/2026-04-03-ai-chat-plugin-design.md` — Segment 1

---

### Task 1: Extend CMake to install `github-copilot-sdk` and `markdown`

**Files:**
- Modify: `cmake/SetupPySideVenv.cmake` (append after line 90, before the Windows python3.dll section)

- [ ] **Step 1: Add pip install for `github-copilot-sdk` and `markdown` after PySide6 installation**

Append the following section after the PySide6 installation block (after the `message(STATUS "PySide6 ... installed successfully.")` on line 89) and before the `# --- Copy python3.dll` comment on line 92:

```cmake
# --- Install additional Python packages for AI Chat plugin -------------------
set(_extra_packages "github-copilot-sdk" "markdown>=3.6")

foreach (_pkg IN LISTS _extra_packages)
    string(REGEX REPLACE "[>=<].*" "" _pkg_name "${_pkg}")
    execute_process(
        COMMAND "${_pyvenv_pip}" show "${_pkg_name}"
        OUTPUT_QUIET
        ERROR_QUIET
        RESULT_VARIABLE _pkg_check_result)
    if (NOT _pkg_check_result EQUAL 0)
        message(STATUS "Installing ${_pkg} into venv...")
        execute_process(
            COMMAND "${_pyvenv_pip}" install "${_pkg}" --quiet
            RESULT_VARIABLE _pkg_install_result)
        if (NOT _pkg_install_result EQUAL 0)
            message(
                WARNING
                    "Failed to install ${_pkg} (exit code ${_pkg_install_result}). "
                    "AI Chat plugin may not function correctly.")
        else ()
            message(STATUS "${_pkg_name} installed successfully.")
        endif ()
    else ()
        message(STATUS "${_pkg_name} already installed, skipping.")
    endif ()
endforeach ()
```

Key decisions:
- Use `WARNING` not `FATAL_ERROR` — these are optional for the AI Chat plugin, not required for the core app build.
- Check each package individually with `pip show` to avoid redundant reinstalls.
- `markdown>=3.6` pins minimum version for stability.

- [ ] **Step 2: Run cmake configure to verify**

Run:
```bash
cmake -S . -B build -G Ninja
```
Expected: Configure succeeds with messages like:
```
-- github-copilot-sdk already installed, skipping.
-- markdown already installed, skipping.
```
Or on first run:
```
-- Installing github-copilot-sdk into venv...
-- github-copilot-sdk installed successfully.
-- Installing markdown>=3.6 into venv...
-- markdown installed successfully.
```

- [ ] **Step 3: Verify packages are importable**

Run from the pyvenv Python:
```bash
pyvenv/Scripts/python.exe -c "import copilot; print('copilot OK'); import markdown; print('markdown OK')"
```
Expected: Both print OK without import errors.

- [ ] **Step 4: Commit**

```bash
git add cmake/SetupPySideVenv.cmake
git commit -m "build(cmake): install github-copilot-sdk and markdown in pyvenv

Extend SetupPySideVenv.cmake to install additional Python packages needed
by the AI Chat plugin. Uses WARNING instead of FATAL_ERROR since these
are optional — core app builds without them.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 2: Extract shared theme to `plugins/_shared/`

**Files:**
- Create: `plugins/_shared/__init__.py`
- Create: `plugins/_shared/plugin_theme.py` (moved from `demo_ui_plugin/plugin_theme.py`)
- Modify: `plugins/demo_ui_plugin/plugin_theme.py` (replace with re-export shim)
- Modify: `plugins/selection_demo_plugin/__init__.py:46` (update import)

- [ ] **Step 1: Create `plugins/_shared/__init__.py`**

```python
"""Shared utilities for OpenGeoLab plugins."""
```

- [ ] **Step 2: Create `plugins/_shared/plugin_theme.py`**

Copy the entire contents of `plugins/demo_ui_plugin/plugin_theme.py` (142 lines) to `plugins/_shared/plugin_theme.py`. The content is identical — `ThemeColors` dataclass, `DARK`/`LIGHT` constants, `detect_dark_mode()`, `current_theme()`, `build_stylesheet()`.

```bash
cp plugins/demo_ui_plugin/plugin_theme.py plugins/_shared/plugin_theme.py
```

- [ ] **Step 3: Replace `plugins/demo_ui_plugin/plugin_theme.py` with re-export shim**

Replace the entire file with:

```python
"""Backwards-compatible re-export — prefer importing from _shared directly."""
from __future__ import annotations

from _shared.plugin_theme import (  # noqa: F401
    DARK,
    LIGHT,
    ThemeColors,
    build_stylesheet,
    current_theme,
    detect_dark_mode,
)
```

This preserves the public API so any existing code importing from `demo_ui_plugin.plugin_theme` continues to work.

- [ ] **Step 4: Update `plugins/selection_demo_plugin/__init__.py` import**

Change line 46 from:
```python
    from demo_ui_plugin.plugin_theme import build_stylesheet, current_theme
```
to:
```python
    from _shared.plugin_theme import build_stylesheet, current_theme
```

- [ ] **Step 5: Verify existing plugins still import correctly**

Run from the plugins directory to simulate the runtime import path:
```bash
cd plugins && ../pyvenv/Scripts/python.exe -c "
import sys; sys.path.insert(0, '.')
from _shared.plugin_theme import ThemeColors, DARK, LIGHT, build_stylesheet
print('_shared OK:', type(DARK))
from demo_ui_plugin.plugin_theme import build_stylesheet as bs2
print('demo_ui_plugin re-export OK:', bs2 is build_stylesheet)
"
```
Expected:
```
_shared OK: <class '_shared.plugin_theme.ThemeColors'>
demo_ui_plugin re-export OK: True
```

Note: `current_theme()` and `detect_dark_mode()` require a QApplication, so we only verify the static imports here.

- [ ] **Step 6: Commit**

```bash
git add plugins/_shared/ plugins/demo_ui_plugin/plugin_theme.py plugins/selection_demo_plugin/__init__.py
git commit -m "refactor(plugins): extract shared plugin_theme to _shared package

Move ThemeColors, DARK/LIGHT palettes, and build_stylesheet() into
plugins/_shared/plugin_theme.py for cross-plugin reuse. The original
demo_ui_plugin/plugin_theme.py becomes a backwards-compatible re-export
shim. selection_demo_plugin updated to import from _shared directly.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 3: Create plugin skeleton — `__init__.py`, `__main__.py`, `scene_tools.py`

**Files:**
- Create: `plugins/ai_chat_plugin/__init__.py`
- Create: `plugins/ai_chat_plugin/__main__.py`
- Create: `plugins/ai_chat_plugin/scene_tools.py`

- [ ] **Step 1: Create `plugins/ai_chat_plugin/scene_tools.py`**

This is the data layer — no PySide6 dependency. Implements the cached 4-layer progressive discovery API as specified in the design spec.

```python
"""Cached hierarchical schema from opengeolab_pywrapper.describe().

Provides four layers of progressive discovery:
  Layer 0 — list_modules():                module names + descriptions
  Layer 1 — describe_module(name):         action names + descriptions
  Layer 2 — describe_action(mod, action):  full param/return schema
  Layer 3 — execute_action(mod, act, p):   execute via pywrapper.process()

In standalone mode (pywrapper unavailable), discovery functions return
empty results and execute_action returns an error dict.
"""
from __future__ import annotations

import json

try:
    import opengeolab_pywrapper as _wrapper

    HOSTED_MODE = True
except ImportError:
    _wrapper = None  # type: ignore[assignment]
    HOSTED_MODE = False

_cached_schema: dict | None = None


def _ensure_schema() -> dict:
    """Fetch and cache the full describe() schema on first call."""
    global _cached_schema
    if _cached_schema is None and HOSTED_MODE:
        _cached_schema = json.loads(_wrapper.describe())
    return _cached_schema or {}


def invalidate_cache() -> None:
    """Force re-fetch of the schema on next access."""
    global _cached_schema
    _cached_schema = None


def list_modules() -> list[dict]:
    """Layer 0: module names and descriptions."""
    schema = _ensure_schema()
    return [
        {"name": m["name"], "description": m.get("description", "")}
        for m in schema.get("modules", [])
    ]


def describe_module(module_name: str) -> dict | None:
    """Layer 1: action names and one-line descriptions for a module."""
    schema = _ensure_schema()
    for m in schema.get("modules", []):
        if m["name"] == module_name:
            return {
                "module": m["name"],
                "description": m.get("description", ""),
                "actions": [
                    {"name": a["name"], "description": a.get("description", "")}
                    for a in m.get("actions", [])
                ],
            }
    return None


def describe_action(module_name: str, action_name: str) -> dict | None:
    """Layer 2: full param/return schema for one action."""
    schema = _ensure_schema()
    for m in schema.get("modules", []):
        if m["name"] == module_name:
            for a in m.get("actions", []):
                if a["name"] == action_name:
                    return a
    return None


def execute_action(
    module: str, action: str, params: dict, progress_callback=None
) -> dict:
    """Layer 3: execute an action via opengeolab_pywrapper.process()."""
    if not HOSTED_MODE:
        return {"ok": False, "error": "Not in hosted mode — pywrapper unavailable"}
    request = json.dumps({"module": module, "action": action, "param": params})
    return json.loads(_wrapper.process(request, progress_callback))
```

- [ ] **Step 2: Create `plugins/ai_chat_plugin/__init__.py`**

```python
"""AI Chat Plugin — Chat with AI powered by GitHub Copilot SDK.

Segment 1 provides an Action Debugger window for browsing and executing
OpenGeoLab commands. Full chat functionality is added in Segment 2.
"""
from __future__ import annotations

_active_windows: list = []


def describe_plugin() -> dict:
    """Return plugin metadata for the runtime discovery system."""
    return {
        "name": "AI Chat",
        "description": "Chat with AI powered by GitHub Copilot SDK.",
        "hasUI": True,
    }


def launch_ui() -> dict:
    """Show the AI Chat plugin window (Action Debugger in Segment 1)."""
    from PySide6.QtCore import Qt
    from PySide6.QtWidgets import QApplication

    application = QApplication.instance()
    if application is None:
        return {
            "ok": False,
            "message": "No QApplication instance.",
        }

    from ai_chat_plugin.action_debugger import ActionDebuggerWindow

    window = ActionDebuggerWindow(embedded=True)
    window.setAttribute(Qt.WA_DeleteOnClose)
    window.destroyed.connect(lambda: _active_windows.remove(window))
    _active_windows.append(window)
    window.show()
    return {"ok": True, "message": "AI Chat (Action Debugger) launched."}
```

- [ ] **Step 3: Create `plugins/ai_chat_plugin/__main__.py`**

```python
"""Standalone entry point: python -m ai_chat_plugin.

Launches the Action Debugger without requiring the C++ host application.
Scene tools are unavailable in standalone mode.
"""
from __future__ import annotations

import sys


def main() -> None:
    """Create a QApplication and show the Action Debugger window."""
    from PySide6.QtWidgets import QApplication

    app = QApplication.instance()
    if app is None:
        app = QApplication(sys.argv)

    from ai_chat_plugin.action_debugger import ActionDebuggerWindow

    window = ActionDebuggerWindow(embedded=False)
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
```

- [ ] **Step 4: Verify scene_tools imports in standalone mode**

```bash
cd plugins && ../pyvenv/Scripts/python.exe -c "
import sys; sys.path.insert(0, '.')
from ai_chat_plugin.scene_tools import (
    HOSTED_MODE, list_modules, describe_module, describe_action, execute_action
)
print('HOSTED_MODE:', HOSTED_MODE)
print('list_modules():', list_modules())
print('describe_module(\"x\"):', describe_module('x'))
result = execute_action('geometry', 'create_box', {})
print('execute_action result:', result)
"
```
Expected (standalone — no pywrapper):
```
HOSTED_MODE: False
list_modules(): []
describe_module("x"): None
execute_action result: {'ok': False, 'error': 'Not in hosted mode — pywrapper unavailable'}
```

- [ ] **Step 5: Commit**

```bash
git add plugins/ai_chat_plugin/__init__.py plugins/ai_chat_plugin/__main__.py plugins/ai_chat_plugin/scene_tools.py
git commit -m "feat(ai-chat): create plugin skeleton with cached scene_tools API

Add ai_chat_plugin package with:
- __init__.py: describe_plugin() + launch_ui() following plugin contract
- __main__.py: standalone entry (python -m ai_chat_plugin)
- scene_tools.py: cached 4-layer progressive discovery API from
  pywrapper.describe() — gracefully returns empty in standalone mode

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 4: Create `json_highlighter.py` — reusable JSON syntax highlighter

**Files:**
- Create: `plugins/ai_chat_plugin/json_highlighter.py`

- [ ] **Step 1: Create `plugins/ai_chat_plugin/json_highlighter.py`**

```python
"""Reusable JSON syntax highlighter for QTextBrowser / QTextEdit.

Uses theme colors from _shared.plugin_theme. Highlights:
- Keys (accent color)
- Strings (green / success)
- Numbers (orange / warning)
- Booleans and null (purple / accent_light)
- Braces, brackets, colons, commas (muted / text_secondary)
"""
from __future__ import annotations

import re

from PySide6.QtCore import Qt
from PySide6.QtGui import QColor, QFont, QSyntaxHighlighter, QTextCharFormat

from _shared.plugin_theme import ThemeColors


class JsonHighlighter(QSyntaxHighlighter):
    """QSyntaxHighlighter subclass for JSON content."""

    def __init__(self, parent=None, *, theme: ThemeColors | None = None) -> None:
        super().__init__(parent)
        self._build_formats(theme)

    def _build_formats(self, theme: ThemeColors | None) -> None:
        """Construct text formats from theme colors."""
        if theme is None:
            from _shared.plugin_theme import DARK

            theme = DARK

        self._key_fmt = self._make_format(theme.accent, bold=True)
        self._string_fmt = self._make_format(theme.success)
        self._number_fmt = self._make_format(theme.warning)
        self._bool_null_fmt = self._make_format(theme.accent_light, bold=True)
        self._brace_fmt = self._make_format(theme.text_secondary)

        # Patterns applied in order; later matches do NOT override earlier ones
        # because we skip already-formatted regions.
        self._rules: list[tuple[re.Pattern, QTextCharFormat]] = [
            # Keys: "key": (captured as the quoted part before a colon)
            (re.compile(r'"[^"\\]*(?:\\.[^"\\]*)*"\s*(?=:)'), self._key_fmt),
            # Strings: "value" (any double-quoted string not followed by colon)
            (re.compile(r'"[^"\\]*(?:\\.[^"\\]*)*"'), self._string_fmt),
            # Numbers: integers and floats (including negative and scientific)
            (re.compile(r'\b-?(?:0|[1-9]\d*)(?:\.\d+)?(?:[eE][+-]?\d+)?\b'), self._number_fmt),
            # Booleans and null
            (re.compile(r'\b(?:true|false|null)\b'), self._bool_null_fmt),
            # Structural characters
            (re.compile(r'[{}\[\]:,]'), self._brace_fmt),
        ]

    @staticmethod
    def _make_format(color_hex: str, bold: bool = False) -> QTextCharFormat:
        fmt = QTextCharFormat()
        fmt.setForeground(QColor(color_hex))
        if bold:
            fmt.setFontWeight(QFont.Weight.Bold)
        return fmt

    def highlightBlock(self, text: str) -> None:
        """Apply highlighting rules to a single block of text."""
        # Track which character positions are already formatted
        formatted = [False] * len(text)

        for pattern, fmt in self._rules:
            for match in pattern.finditer(text):
                start, end = match.start(), match.end()
                # Skip if any character in this range is already formatted
                if any(formatted[start:end]):
                    continue
                self.setFormat(start, end - start, fmt)
                for i in range(start, end):
                    formatted[i] = True
```

- [ ] **Step 2: Verify the highlighter can be instantiated (standalone smoke test)**

```bash
cd plugins && ../pyvenv/Scripts/python.exe -c "
import sys; sys.path.insert(0, '.')
from PySide6.QtWidgets import QApplication
from PySide6.QtGui import QTextDocument
app = QApplication(sys.argv)
from ai_chat_plugin.json_highlighter import JsonHighlighter
from _shared.plugin_theme import DARK
doc = QTextDocument()
doc.setPlainText('{\"key\": \"value\", \"num\": 42, \"ok\": true}')
hl = JsonHighlighter(doc, theme=DARK)
print('JsonHighlighter created OK, block count:', doc.blockCount())
app.quit()
"
```
Expected:
```
JsonHighlighter created OK, block count: 1
```

- [ ] **Step 3: Commit**

```bash
git add plugins/ai_chat_plugin/json_highlighter.py
git commit -m "feat(ai-chat): add reusable JSON syntax highlighter

JsonHighlighter (QSyntaxHighlighter) highlights keys, strings, numbers,
booleans/null, and structural characters using theme colors from
_shared.plugin_theme. Will be reused for action detail panel, response
display, and Segment 2 tool call cards.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```
