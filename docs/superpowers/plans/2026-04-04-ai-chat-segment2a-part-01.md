# AI Chat Plugin — Segment 2a: Part 1 of 3

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **Recommended models:** Use Claude Opus 4.6 for design or plan revisions. Use GPT-5.4 for implementation, debugging, verification, and code-review subtasks.

**Goal:** Build the Python backend core for the AI Chat feature — dependencies, prompt files, Markdown converter, chat message model, and tool handler definitions.

**Architecture:** Python-side data model and utilities that will be wired into the QThread-based Copilot SDK integration (Part 2) and QML UI (Part 3).  All new files live under `plugins/ai_chat_plugin/`.  `ChatMessageModel` is a `QAbstractListModel` powering the QML `ListView`.  `markdown_converter` is a pure function.  `tool_handlers` wrap `scene_tools` for the SDK.

**Tech Stack:** Python 3.13, PySide6 6.9, `markdown` library, `pydantic` (SDK dependency), `github-copilot-sdk`

**Spec:** `docs/superpowers/specs/2026-04-04-ai-chat-segment2a-chat-core-design.md`

---

### Task 1: Install Python Dependencies

**Files:**
- Modify: `cmake/SetupPySideVenv.cmake` (if it exists — add pip install lines)
- Otherwise: manual pip install into `pyvenv/`

- [ ] **Step 1: Install `markdown` and `github-copilot-sdk` into pyvenv**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
.\pyvenv\Scripts\pip.exe install "markdown>=3.6" "github-copilot-sdk"
```

Expected: both packages install successfully.  `pydantic` is pulled in as a transitive dependency of `github-copilot-sdk`.

- [ ] **Step 2: Verify imports work**

```powershell
.\pyvenv\Scripts\python.exe -c "import markdown; print(markdown.__version__)"
.\pyvenv\Scripts\python.exe -c "from copilot import CopilotClient, define_tool; print('SDK OK')"
```

Expected: version number printed, then `SDK OK`.

- [ ] **Step 3: Commit**

```powershell
git add -u  # Only if cmake file was changed
git commit -m "build(ai-chat): install markdown and github-copilot-sdk into pyvenv

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

> **Note:** If dependencies are already installed from a prior session, skip this task.

---

### Task 2: Create Prompt Files

**Files:**
- Create: `plugins/ai_chat_plugin/prompts/system_prompt.md`
- Create: `plugins/ai_chat_plugin/prompts/skill.md`

- [ ] **Step 1: Create the prompts directory**

```powershell
New-Item -ItemType Directory -Path "plugins/ai_chat_plugin/prompts" -Force
```

- [ ] **Step 2: Create `system_prompt.md`**

Create file `plugins/ai_chat_plugin/prompts/system_prompt.md`:

```markdown
You are the OpenGeoLab AI Assistant. You help users interact with the
OpenGeoLab CAD application through natural language.

You have access to tools that let you discover and execute commands:
1. Use `list_modules` to see available modules
2. Use `describe_module` to learn about a module's actions and their parameter schemas
3. Use `execute_action` to run commands with specific parameters

Rules:
- Always discover actions before executing them — never guess action names or parameter schemas
- Confirm destructive or irreversible operations with the user before executing
- Report results clearly and suggest possible next steps
- When an action fails, explain the error and suggest corrections
- Use the parameter schema from describe_module to construct correct parameters
```

- [ ] **Step 3: Create `skill.md`**

Create file `plugins/ai_chat_plugin/prompts/skill.md`:

```markdown
# OpenGeoLab Command Protocol

## Request Format

All OpenGeoLab commands follow a JSON protocol:
- Request: `{module, action, param}` → Response: `{ok, result/errors}`

## Discovery Workflow

1. `list_modules()` → returns array of `{name, description}` for all modules
2. `describe_module(module_name)` → returns `{module, description, actions}` where each action has `{name, description}`
3. `execute_action(module, action, params)` → returns the action result with `{ok: true/false, ...}`

Always follow this order: discover modules → describe the relevant module → execute with correct params.

## Available Modules (discovered at runtime)

Use `list_modules` to discover.  Common modules include:
- **geometry**: Create shapes (create_box, create_cylinder, create_sphere, etc.)
- **mesh**: Generate meshes from geometry (mesh_shape, etc.)
- **scene**: Selection, camera, labels, hover
- **io**: Import/export files

## Parameter Constraints

- Parameter names and types must match the schema exactly (case-sensitive)
- `shapeId` values come from geometry creation results — never fabricate them
- Entity types: `GeoSolid`, `GeoFace`, `GeoEdge`, `GeoVertex`, `MeshNode`, `MeshEdge`, `MeshElement`
- Numeric parameters use standard JSON number format (no units)

## Error Handling

- If `ok` is `false`, the `errors` field contains an error description
- Common errors: missing required parameter, invalid shapeId, unknown action name
- On error, re-check the parameter schema with `describe_module` before retrying
```

- [ ] **Step 4: Verify files exist**

```powershell
Get-ChildItem plugins/ai_chat_plugin/prompts/
```

Expected: `skill.md` and `system_prompt.md` listed.

- [ ] **Step 5: Commit**

```powershell
git add plugins/ai_chat_plugin/prompts/system_prompt.md plugins/ai_chat_plugin/prompts/skill.md
git commit -m "feat(ai-chat): add system prompt and skill document for Copilot SDK

System prompt defines the AI assistant persona and tool usage rules.
Skill document teaches the command protocol, discovery workflow, and
parameter constraints.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 3: Create Markdown Converter

**Files:**
- Create: `plugins/ai_chat_plugin/markdown_converter.py`
- Create: `plugins/ai_chat_plugin/tests/__init__.py`
- Create: `plugins/ai_chat_plugin/tests/test_markdown_converter.py`

- [ ] **Step 1: Create test directory**

```powershell
New-Item -ItemType Directory -Path "plugins/ai_chat_plugin/tests" -Force
New-Item -ItemType File -Path "plugins/ai_chat_plugin/tests/__init__.py" -Force
```

- [ ] **Step 2: Write the failing tests**

Create file `plugins/ai_chat_plugin/tests/test_markdown_converter.py`:

```python
"""Tests for markdown_converter module."""
from __future__ import annotations

import pytest


def test_basic_markdown_to_html():
    """Bold and italic produce correct HTML tags."""
    from ai_chat_plugin.markdown_converter import markdown_to_html

    result = markdown_to_html("**bold** and *italic*")
    assert "<strong>bold</strong>" in result
    assert "<em>italic</em>" in result


def test_fenced_code_block():
    """Fenced code blocks produce <pre><code> tags."""
    from ai_chat_plugin.markdown_converter import markdown_to_html

    md = "```python\nprint('hello')\n```"
    result = markdown_to_html(md)
    assert "<code" in result
    assert "print(" in result


def test_table_extension():
    """Tables produce <table> tags."""
    from ai_chat_plugin.markdown_converter import markdown_to_html

    md = "| A | B |\n|---|---|\n| 1 | 2 |"
    result = markdown_to_html(md)
    assert "<table>" in result or "<table" in result


def test_contains_style_block():
    """Output includes a <style> block for theming."""
    from ai_chat_plugin.markdown_converter import markdown_to_html

    result = markdown_to_html("hello")
    assert "<style>" in result


def test_dark_mode_vs_light_mode():
    """Dark and light modes produce different CSS."""
    from ai_chat_plugin.markdown_converter import markdown_to_html

    dark = markdown_to_html("hello", dark_mode=True)
    light = markdown_to_html("hello", dark_mode=False)
    # Both should have style blocks but different colors
    assert "<style>" in dark
    assert "<style>" in light
    assert dark != light


def test_raw_html_escaped():
    """Raw HTML in input is NOT rendered as real HTML (security)."""
    from ai_chat_plugin.markdown_converter import markdown_to_html

    result = markdown_to_html('<script>alert("xss")</script>')
    # The markdown library without 'html' extension escapes raw HTML
    assert "<script>" not in result or "&lt;script&gt;" in result


def test_empty_input():
    """Empty string produces a style block with empty body."""
    from ai_chat_plugin.markdown_converter import markdown_to_html

    result = markdown_to_html("")
    assert "<style>" in result
```

- [ ] **Step 3: Run tests to verify they fail**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
.\pyvenv\Scripts\python.exe -m pytest plugins/ai_chat_plugin/tests/test_markdown_converter.py -v
```

Expected: all tests FAIL with `ModuleNotFoundError: No module named 'ai_chat_plugin.markdown_converter'`.

- [ ] **Step 4: Implement `markdown_converter.py`**

Create file `plugins/ai_chat_plugin/markdown_converter.py`:

```python
"""Convert Markdown text to themed HTML for QML TextArea(RichText).

Uses the ``markdown`` library with ``fenced_code`` and ``tables`` extensions.
Wraps output in a ``<style>`` block with colours derived from *dark_mode*.

The ``markdown`` library escapes raw HTML by default (no ``html`` extension
enabled), so embedded ``<script>`` tags are rendered as literal text.
"""
from __future__ import annotations

import markdown as _md


def markdown_to_html(text: str, dark_mode: bool = True) -> str:
    """Convert Markdown to a themed HTML fragment.

    Parameters
    ----------
    text:
        Raw Markdown text.
    dark_mode:
        When ``True`` (default), use dark-theme colours.

    Returns
    -------
    str
        HTML string with an embedded ``<style>`` block.
    """
    html_body = _md.markdown(
        text,
        extensions=["fenced_code", "tables"],
    )
    css = _build_css(dark_mode)
    return f"<style>{css}</style>{html_body}"


def _build_css(dark_mode: bool) -> str:
    """Build a CSS block adapted to the current theme."""
    if dark_mode:
        text_color = "#e0e0e0"
        bg_code = "#2d2d2d"
        border_color = "#444444"
        link_color = "#58a6ff"
        table_alt_bg = "#2a2a2a"
    else:
        text_color = "#1a1a1a"
        bg_code = "#f5f5f5"
        border_color = "#d0d0d0"
        link_color = "#0969da"
        table_alt_bg = "#f0f0f0"

    return f"""
        body {{
            color: {text_color};
            font-family: -apple-system, 'Segoe UI', sans-serif;
            font-size: 13px;
            line-height: 1.5;
        }}
        pre {{
            background: {bg_code};
            border: 1px solid {border_color};
            border-radius: 6px;
            padding: 8px 12px;
            overflow-x: auto;
            font-family: 'Cascadia Code', 'Consolas', monospace;
            font-size: 12px;
        }}
        code {{
            background: {bg_code};
            border-radius: 3px;
            padding: 1px 4px;
            font-family: 'Cascadia Code', 'Consolas', monospace;
            font-size: 12px;
        }}
        pre code {{
            background: transparent;
            padding: 0;
        }}
        a {{
            color: {link_color};
            text-decoration: none;
        }}
        table {{
            border-collapse: collapse;
            width: 100%;
            margin: 8px 0;
        }}
        th, td {{
            border: 1px solid {border_color};
            padding: 4px 8px;
            text-align: left;
        }}
        tr:nth-child(even) {{
            background: {table_alt_bg};
        }}
    """
```

- [ ] **Step 5: Run tests to verify they pass**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
.\pyvenv\Scripts\python.exe -m pytest plugins/ai_chat_plugin/tests/test_markdown_converter.py -v
```

Expected: all 7 tests PASS.

- [ ] **Step 6: Commit**

```powershell
git add plugins/ai_chat_plugin/markdown_converter.py plugins/ai_chat_plugin/tests/__init__.py plugins/ai_chat_plugin/tests/test_markdown_converter.py
git commit -m "feat(ai-chat): add Markdown-to-HTML converter with theme support

Pure function converting Markdown to themed HTML for QML RichText display.
Uses fenced_code and tables extensions.  Raw HTML is escaped by default
for security.  Dark/light theme CSS included.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 4: Create ChatMessageModel

**Files:**
- Create: `plugins/ai_chat_plugin/chat_message_model.py`
- Create: `plugins/ai_chat_plugin/tests/test_chat_message_model.py`

- [ ] **Step 1: Write the failing tests**

Create file `plugins/ai_chat_plugin/tests/test_chat_message_model.py`:

```python
"""Tests for ChatMessageModel — QAbstractListModel for chat messages."""
from __future__ import annotations

import sys
import pytest

# PySide6 requires a QApplication to exist before using any Qt types.
_app = None

def _ensure_app():
    global _app
    if _app is None:
        from PySide6.QtWidgets import QApplication
        _app = QApplication.instance() or QApplication(sys.argv)
    return _app


@pytest.fixture(autouse=True)
def qt_app():
    """Ensure QApplication exists for all tests."""
    _ensure_app()


def _make_model():
    from ai_chat_plugin.chat_message_model import ChatMessageModel
    return ChatMessageModel()


class TestAppendMessage:
    def test_append_user_message(self):
        model = _make_model()
        row = model.appendMessage("user", "Hello!")
        assert row == 0
        assert model.rowCount() == 1

    def test_append_returns_incrementing_rows(self):
        model = _make_model()
        r0 = model.appendMessage("user", "msg1")
        r1 = model.appendMessage("assistant", "msg2")
        assert r0 == 0
        assert r1 == 1
        assert model.rowCount() == 2

    def test_data_roles(self):
        from PySide6.QtCore import Qt
        from ai_chat_plugin.chat_message_model import ChatMessageModel

        model = _make_model()
        model.appendMessage("user", "test content")
        idx = model.index(0, 0)

        role_names = model.roleNames()
        # Find the role number for 'msgType'
        msg_type_role = None
        content_role = None
        for role_num, name in role_names.items():
            if name == b"msgType":
                msg_type_role = role_num
            elif name == b"content":
                content_role = role_num

        assert msg_type_role is not None
        assert model.data(idx, msg_type_role) == "user"
        assert model.data(idx, content_role) == "test content"

    def test_append_tool_message(self):
        model = _make_model()
        row = model.appendMessage(
            "tool", "",
            toolName="list_modules",
            toolCallId="tc_123",
            toolStatus="running",
        )
        assert row == 0
        assert model.rowCount() == 1

    def test_append_askuser_message(self):
        model = _make_model()
        row = model.appendMessage(
            "askUser", "Which option?",
            choices=["A", "B", "C"],
        )
        assert row == 0


class TestStreamingUpdate:
    def test_append_to_last_assistant(self):
        model = _make_model()
        model.appendMessage("assistant", "", msgId="msg_1")
        model.appendToLastAssistant("Hello ")
        model.appendToLastAssistant("world")

        from ai_chat_plugin.chat_message_model import ChatMessageModel
        idx = model.index(0, 0)
        role_names = model.roleNames()
        content_role = [r for r, n in role_names.items() if n == b"content"][0]
        assert model.data(idx, content_role) == "Hello world"

    def test_finalize_assistant(self):
        model = _make_model()
        model.appendMessage("assistant", "", msgId="msg_1")
        model.appendToLastAssistant("raw text")
        model.finalizeAssistant("msg_1", "<p>formatted</p>")

        idx = model.index(0, 0)
        role_names = model.roleNames()
        content_role = [r for r, n in role_names.items() if n == b"content"][0]
        is_html_role = [r for r, n in role_names.items() if n == b"isHtml"][0]
        assert model.data(idx, content_role) == "<p>formatted</p>"
        assert model.data(idx, is_html_role) is True

    def test_finalize_wrong_id_no_crash(self):
        """Finalizing a non-existent msgId should be a no-op."""
        model = _make_model()
        model.appendMessage("assistant", "", msgId="msg_1")
        model.finalizeAssistant("msg_wrong", "<p>oops</p>")
        # Should not crash, content unchanged
        idx = model.index(0, 0)
        role_names = model.roleNames()
        content_role = [r for r, n in role_names.items() if n == b"content"][0]
        assert model.data(idx, content_role) == ""


class TestToolUpdate:
    def test_update_tool_status(self):
        model = _make_model()
        model.appendMessage(
            "tool", "",
            toolName="execute_action",
            toolCallId="tc_456",
            toolStatus="running",
        )
        model.updateToolStatus("tc_456", "success", '{"ok": true}')

        idx = model.index(0, 0)
        role_names = model.roleNames()
        status_role = [r for r, n in role_names.items() if n == b"toolStatus"][0]
        result_role = [r for r, n in role_names.items() if n == b"toolResult"][0]
        assert model.data(idx, status_role) == "success"
        assert model.data(idx, result_role) == '{"ok": true}'

    def test_update_unknown_tool_no_crash(self):
        model = _make_model()
        model.updateToolStatus("tc_unknown", "error", "not found")
        # Should not crash


class TestAskUserUpdate:
    def test_mark_answered(self):
        model = _make_model()
        model.appendMessage("askUser", "Pick one", choices=["A", "B"])
        model.markAskUserAnswered(0)

        idx = model.index(0, 0)
        role_names = model.roleNames()
        answered_role = [r for r, n in role_names.items() if n == b"answered"][0]
        assert model.data(idx, answered_role) is True


class TestClear:
    def test_clear_removes_all(self):
        model = _make_model()
        model.appendMessage("user", "a")
        model.appendMessage("assistant", "b")
        assert model.rowCount() == 2
        model.clear()
        assert model.rowCount() == 0
```

- [ ] **Step 2: Run tests to verify they fail**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
.\pyvenv\Scripts\python.exe -m pytest plugins/ai_chat_plugin/tests/test_chat_message_model.py -v
```

Expected: FAIL with `ModuleNotFoundError: No module named 'ai_chat_plugin.chat_message_model'`.

- [ ] **Step 3: Implement `chat_message_model.py`**

Create file `plugins/ai_chat_plugin/chat_message_model.py`:

```python
"""QAbstractListModel for chat messages displayed in the QML ListView.

Each row represents a message with roles matching the QML delegate expectations.
Row identification uses ``msgId`` for streaming updates rather than last-index
to prevent race conditions when tool rows interleave during streaming.
"""
from __future__ import annotations

import uuid
from typing import Any

from PySide6.QtCore import (
    QAbstractListModel,
    QModelIndex,
    Qt,
    Slot,
)


class _Roles:
    """Custom role enum starting after Qt.UserRole."""

    MSG_ID = Qt.UserRole + 1
    MSG_TYPE = Qt.UserRole + 2
    CONTENT = Qt.UserRole + 3
    TOOL_NAME = Qt.UserRole + 4
    TOOL_CALL_ID = Qt.UserRole + 5
    TOOL_STATUS = Qt.UserRole + 6
    TOOL_RESULT = Qt.UserRole + 7
    CHOICES = Qt.UserRole + 8
    ANSWERED = Qt.UserRole + 9
    IS_HTML = Qt.UserRole + 10


_ROLE_NAMES = {
    _Roles.MSG_ID: b"msgId",
    _Roles.MSG_TYPE: b"msgType",
    _Roles.CONTENT: b"content",
    _Roles.TOOL_NAME: b"toolName",
    _Roles.TOOL_CALL_ID: b"toolCallId",
    _Roles.TOOL_STATUS: b"toolStatus",
    _Roles.TOOL_RESULT: b"toolResult",
    _Roles.CHOICES: b"choices",
    _Roles.ANSWERED: b"answered",
    _Roles.IS_HTML: b"isHtml",
}


def _new_row(
    msg_type: str,
    content: str,
    *,
    msg_id: str = "",
    tool_name: str = "",
    tool_call_id: str = "",
    tool_status: str = "",
    tool_result: str = "",
    choices: list[str] | None = None,
    answered: bool = False,
    is_html: bool = False,
) -> dict[int, Any]:
    """Build a role→value dict for one row."""
    return {
        _Roles.MSG_ID: msg_id or str(uuid.uuid4()),
        _Roles.MSG_TYPE: msg_type,
        _Roles.CONTENT: content,
        _Roles.TOOL_NAME: tool_name,
        _Roles.TOOL_CALL_ID: tool_call_id,
        _Roles.TOOL_STATUS: tool_status,
        _Roles.TOOL_RESULT: tool_result,
        _Roles.CHOICES: choices or [],
        _Roles.ANSWERED: answered,
        _Roles.IS_HTML: is_html,
    }


class ChatMessageModel(QAbstractListModel):
    """List model backing the chat message ListView in QML."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._rows: list[dict[int, Any]] = []

    # -- QAbstractListModel interface ------------------------------------------

    def rowCount(self, parent: QModelIndex = QModelIndex()) -> int:  # noqa: N802
        return len(self._rows)

    def data(self, index: QModelIndex, role: int = Qt.DisplayRole) -> Any:
        if not index.isValid() or not 0 <= index.row() < len(self._rows):
            return None
        return self._rows[index.row()].get(role)

    def roleNames(self) -> dict[int, bytes]:  # noqa: N802
        return _ROLE_NAMES

    # -- Public API (called from ChatBackend) ----------------------------------

    @Slot(str, str, result=int)
    def appendMessage(  # noqa: N802
        self,
        msg_type: str,
        content: str,
        **kwargs: Any,
    ) -> int:
        """Append a new message row.  Returns the row index."""
        # Map pythonic kwarg names to _new_row parameters.
        row_kwargs: dict[str, Any] = {}
        _kwarg_map = {
            "msgId": "msg_id",
            "toolName": "tool_name",
            "toolCallId": "tool_call_id",
            "toolStatus": "tool_status",
            "toolResult": "tool_result",
            "choices": "choices",
            "answered": "answered",
            "isHtml": "is_html",
        }
        for camel, snake in _kwarg_map.items():
            if camel in kwargs:
                row_kwargs[snake] = kwargs[camel]
            elif snake in kwargs:
                row_kwargs[snake] = kwargs[snake]

        row = _new_row(msg_type, content, **row_kwargs)
        idx = len(self._rows)
        self.beginInsertRows(QModelIndex(), idx, idx)
        self._rows.append(row)
        self.endInsertRows()
        return idx

    def appendToLastAssistant(self, delta: str) -> None:  # noqa: N802
        """Append streaming delta text to the active assistant row.

        Scans from the end for a row with ``msgType == "assistant"`` and
        ``isHtml == False``.
        """
        for i in range(len(self._rows) - 1, -1, -1):
            row = self._rows[i]
            if row[_Roles.MSG_TYPE] == "assistant" and not row[_Roles.IS_HTML]:
                row[_Roles.CONTENT] += delta
                idx = self.index(i, 0)
                self.dataChanged.emit(idx, idx, [_Roles.CONTENT])
                return

    def finalizeAssistant(self, msg_id: str, html: str) -> None:  # noqa: N802
        """Replace content with HTML for the assistant row matching *msg_id*."""
        for i in range(len(self._rows) - 1, -1, -1):
            row = self._rows[i]
            if row[_Roles.MSG_ID] == msg_id:
                row[_Roles.CONTENT] = html
                row[_Roles.IS_HTML] = True
                idx = self.index(i, 0)
                self.dataChanged.emit(
                    idx, idx, [_Roles.CONTENT, _Roles.IS_HTML]
                )
                return

    def updateToolStatus(  # noqa: N802
        self, tool_call_id: str, status: str, result: str
    ) -> None:
        """Update a tool row identified by *tool_call_id*."""
        for i, row in enumerate(self._rows):
            if row[_Roles.TOOL_CALL_ID] == tool_call_id:
                row[_Roles.TOOL_STATUS] = status
                row[_Roles.TOOL_RESULT] = result
                idx = self.index(i, 0)
                self.dataChanged.emit(
                    idx, idx, [_Roles.TOOL_STATUS, _Roles.TOOL_RESULT]
                )
                return

    def markAskUserAnswered(self, row_index: int) -> None:  # noqa: N802
        """Mark an askUser row as answered."""
        if 0 <= row_index < len(self._rows):
            self._rows[row_index][_Roles.ANSWERED] = True
            idx = self.index(row_index, 0)
            self.dataChanged.emit(idx, idx, [_Roles.ANSWERED])

    @Slot()
    def clear(self) -> None:
        """Remove all rows."""
        if not self._rows:
            return
        self.beginRemoveRows(QModelIndex(), 0, len(self._rows) - 1)
        self._rows.clear()
        self.endRemoveRows()
```

- [ ] **Step 4: Run tests to verify they pass**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
.\pyvenv\Scripts\python.exe -m pytest plugins/ai_chat_plugin/tests/test_chat_message_model.py -v
```

Expected: all 12 tests PASS.

- [ ] **Step 5: Commit**

```powershell
git add plugins/ai_chat_plugin/chat_message_model.py plugins/ai_chat_plugin/tests/test_chat_message_model.py
git commit -m "feat(ai-chat): add ChatMessageModel with streaming support

QAbstractListModel for QML ListView with roles for all message types
(user, assistant, tool, askUser, system).  Streaming uses msgId-based
row tracking (appendToLastAssistant + finalizeAssistant) to prevent
race conditions when tool rows interleave.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 5: Create Tool Handlers

**Files:**
- Create: `plugins/ai_chat_plugin/tool_handlers.py`
- Create: `plugins/ai_chat_plugin/tests/test_tool_handlers.py`

- [ ] **Step 1: Write the failing tests**

Create file `plugins/ai_chat_plugin/tests/test_tool_handlers.py`:

```python
"""Tests for tool_handlers — Copilot SDK tool definitions."""
from __future__ import annotations

import json
from unittest.mock import patch, MagicMock


def test_build_tools_hosted_mode():
    """In hosted mode, build_tools returns 3 tool objects."""
    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.HOSTED_MODE = True
        from ai_chat_plugin.tool_handlers import build_tools
        tools = build_tools()
    assert len(tools) == 3
    names = {t.name for t in tools}
    assert names == {"list_modules", "describe_module", "execute_action"}


def test_build_tools_standalone_mode():
    """In standalone mode, build_tools returns an empty list."""
    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.HOSTED_MODE = False
        from ai_chat_plugin.tool_handlers import build_tools
        tools = build_tools()
    assert len(tools) == 0


def test_list_modules_handler():
    """list_modules tool handler returns JSON string of modules."""
    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.list_modules.return_value = [
            {"name": "geometry", "description": "Shapes"}
        ]
        from ai_chat_plugin.tool_handlers import _list_modules_handler
        result = _list_modules_handler()
    parsed = json.loads(result)
    assert len(parsed) == 1
    assert parsed[0]["name"] == "geometry"


def test_describe_module_handler():
    """describe_module tool handler returns module details."""
    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.describe_module.return_value = {
            "module": "geometry",
            "actions": [{"name": "create_box"}],
        }
        from ai_chat_plugin.tool_handlers import _describe_module_handler
        result = _describe_module_handler("geometry")
    parsed = json.loads(result)
    assert parsed["module"] == "geometry"


def test_execute_action_handler():
    """execute_action tool handler returns action result."""
    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.execute_action.return_value = {"ok": True, "shapeId": "s1"}
        from ai_chat_plugin.tool_handlers import _execute_action_handler
        result = _execute_action_handler("geometry", "create_box", {"size": 1})
    parsed = json.loads(result)
    assert parsed["ok"] is True


def test_execute_action_handler_catches_exception():
    """execute_action returns error JSON when scene_tools raises."""
    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.execute_action.side_effect = RuntimeError("boom")
        from ai_chat_plugin.tool_handlers import _execute_action_handler
        result = _execute_action_handler("geometry", "create_box", {})
    parsed = json.loads(result)
    assert parsed["ok"] is False
    assert "boom" in parsed["error"]


def test_result_truncation():
    """Results exceeding 50 KB are truncated."""
    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        # Return a result with > 50 KB of data
        mock_st.execute_action.return_value = {"ok": True, "data": "x" * 60_000}
        from ai_chat_plugin.tool_handlers import _execute_action_handler
        result = _execute_action_handler("geometry", "big_action", {})
    assert len(result) <= 51_300  # 50 KB + truncation suffix + some overhead
    assert result.endswith('...(truncated)"')
```

- [ ] **Step 2: Run tests to verify they fail**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
.\pyvenv\Scripts\python.exe -m pytest plugins/ai_chat_plugin/tests/test_tool_handlers.py -v
```

Expected: FAIL with `ModuleNotFoundError`.

- [ ] **Step 3: Implement `tool_handlers.py`**

Create file `plugins/ai_chat_plugin/tool_handlers.py`:

```python
"""Copilot SDK tool definitions for OpenGeoLab command integration.

Provides ``build_tools()`` which returns a list of SDK ``Tool`` objects
based on whether the plugin is running in hosted mode (pywrapper available)
or standalone mode (no tools).

Tool handlers are also exposed as module-level functions for testability.
"""
from __future__ import annotations

import json
import traceback

from pydantic import BaseModel, Field

from copilot import define_tool

from ai_chat_plugin import scene_tools

_MAX_RESULT_BYTES = 50 * 1024  # 50 KB


class _DescribeModuleParams(BaseModel):
    module_name: str = Field(description="Name of the module to describe")


class _ExecuteActionParams(BaseModel):
    module: str = Field(description="Module name")
    action: str = Field(description="Action name")
    params: dict = Field(default_factory=dict, description="Action parameters")


def _truncate(text: str) -> str:
    """Truncate text to _MAX_RESULT_BYTES with a suffix."""
    if len(text.encode("utf-8", errors="replace")) <= _MAX_RESULT_BYTES:
        return text
    truncated = text[:_MAX_RESULT_BYTES]
    return truncated + '...(truncated)"'


def _list_modules_handler() -> str:
    """Handler for the list_modules tool."""
    modules = scene_tools.list_modules()
    return json.dumps(modules, default=str)


def _describe_module_handler(module_name: str) -> str:
    """Handler for the describe_module tool."""
    result = scene_tools.describe_module(module_name)
    if result is None:
        return json.dumps({"ok": False, "error": f"Module '{module_name}' not found"})
    return json.dumps(result, default=str)


def _execute_action_handler(module: str, action: str, params: dict) -> str:
    """Handler for the execute_action tool."""
    try:
        result = scene_tools.execute_action(module, action, params)
        text = json.dumps(result, default=str)
        return _truncate(text)
    except Exception:
        return json.dumps({
            "ok": False,
            "error": traceback.format_exc(limit=3),
        })


def build_tools() -> list:
    """Build the list of SDK Tool objects based on current mode.

    Returns an empty list in standalone mode (no scene tools available).
    """
    if not scene_tools.HOSTED_MODE:
        return []

    @define_tool(
        description="List all available OpenGeoLab command modules",
        skip_permission=True,
    )
    def list_modules() -> str:
        return _list_modules_handler()

    @define_tool(
        description="Describe a module's actions and their parameter schemas",
        skip_permission=True,
    )
    def describe_module(params: _DescribeModuleParams) -> str:
        return _describe_module_handler(params.module_name)

    @define_tool(
        description="Execute an OpenGeoLab action with given parameters",
    )
    def execute_action(params: _ExecuteActionParams) -> str:
        return _execute_action_handler(params.module, params.action, params.params)

    return [list_modules, describe_module, execute_action]
```

- [ ] **Step 4: Run tests to verify they pass**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
.\pyvenv\Scripts\python.exe -m pytest plugins/ai_chat_plugin/tests/test_tool_handlers.py -v
```

Expected: all 7 tests PASS.

- [ ] **Step 5: Commit**

```powershell
git add plugins/ai_chat_plugin/tool_handlers.py plugins/ai_chat_plugin/tests/test_tool_handlers.py
git commit -m "feat(ai-chat): add Copilot SDK tool handlers for scene integration

Three tools: list_modules, describe_module, execute_action — wrapping
scene_tools for the SDK.  Includes error catching, result truncation
(50 KB limit), and standalone mode support (empty tools list).

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

**End of Part 1.** Continue to Part 2 for CopilotWorker, ChatBackend, and window refactoring.
