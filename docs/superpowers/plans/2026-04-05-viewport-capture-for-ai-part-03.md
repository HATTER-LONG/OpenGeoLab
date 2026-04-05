# Viewport Capture for AI — Part 3 of 3

> Part 文件：Python tool handler + CopilotWorker attachment support + QML UI — Tasks 7-10.
> 依赖：Part 2 完成（CaptureViewportAction 返回 metadata + base64 image）。

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **Recommended models:** Use Claude Opus 4.6 for design or plan revisions. Use GPT-5.4 for implementation, debugging, verification, and code-review subtasks.

**Goal:** Add Python `capture_viewport` tool for the Copilot SDK, extend `CopilotWorker` to support image attachments via `BlobAttachment`, add QML 📎 button for manual viewport capture, and update the system prompt for vision capabilities.

**Architecture:** The Python tool handler calls `scene_tools.execute_action("scene", "capture_viewport", ...)` to get base64 image + metadata from C++. The image is stored as a `_pending_attachment` on the worker's thread-local state, and the metadata is returned as the tool result. When the SDK sends the next model turn, the pending attachment is included via `session.send(prompt, attachments=[blob])`. For user-triggered captures, ChatBackend calls the C++ action directly and stores the result for the next message send.

**Tech Stack:** Python 3.11+ · Copilot SDK (BlobAttachment) · PySide6/QML · pytest

---

## File Map

| Action | Path | Responsibility |
|--------|------|----------------|
| Tool handler | `plugins/ai_chat_plugin/tool_handlers.py` | New `capture_viewport` tool definition |
| Worker | `plugins/ai_chat_plugin/copilot_worker.py` | Attachment queue + send with attachments |
| Backend | `plugins/ai_chat_plugin/chat_backend.py` | `captureViewport()` Q_INVOKABLE |
| QML input | `plugins/ai_chat_plugin/qml/ChatInputArea.qml` | 📎 button + attachment preview |
| System prompt | `plugins/ai_chat_plugin/prompts/system_prompt.md` | Vision capability instructions |
| Test | `plugins/ai_chat_plugin/tests/test_capture_tool.py` | Tool handler unit tests |

---

### Task 7: Python capture_viewport Tool Handler

**Files:**
- Modify: `plugins/ai_chat_plugin/tool_handlers.py`
- Create: `plugins/ai_chat_plugin/tests/test_capture_tool.py`

- [ ] **Step 1: Write the failing test**

Create `plugins/ai_chat_plugin/tests/test_capture_tool.py`:

```python
"""Tests for the capture_viewport tool handler."""
from __future__ import annotations

import json
from unittest.mock import MagicMock, patch

import pytest


def test_capture_viewport_handler_returns_metadata():
    """The handler should return metadata JSON and store image as pending attachment."""
    mock_result = {
        "ok": True,
        "action": "capture_viewport",
        "image": "iVBORw0KGgoAAAANSUhEUg==",  # fake base64
        "metadata": {
            "viewport": {"width": 1024, "height": 768},
            "camera": {"eye": [0, 0, 50], "target": [0, 0, 0], "up": [0, 1, 0]},
            "visibleShapes": [],
            "selections": [],
            "labels": [],
            "hover": None,
        },
    }

    with patch("ai_chat_plugin.scene_tools.execute_action", return_value=mock_result):
        from ai_chat_plugin.tool_handlers import _capture_viewport_handler

        result_json = _capture_viewport_handler(1024, 768, "png")
        result = json.loads(result_json)

    assert result["viewport"]["width"] == 1024
    assert result["camera"]["eye"] == [0, 0, 50]
    assert result["imageAttached"] is True
    # image field should NOT be in the tool return — it goes to pending attachment
    assert "image" not in result


def test_capture_viewport_handler_no_image():
    """When C++ action returns no image, imageAttached should be false."""
    mock_result = {
        "ok": True,
        "action": "capture_viewport",
        "image": None,
        "imageError": "Capture timed out",
        "metadata": {
            "viewport": {"width": 1024, "height": 768},
            "camera": {"eye": [0, 0, 50], "target": [0, 0, 0], "up": [0, 1, 0]},
            "visibleShapes": [],
            "selections": [],
            "labels": [],
            "hover": None,
        },
    }

    with patch("ai_chat_plugin.scene_tools.execute_action", return_value=mock_result):
        from ai_chat_plugin.tool_handlers import _capture_viewport_handler

        result_json = _capture_viewport_handler(512, 384, "png")
        result = json.loads(result_json)

    assert result["imageAttached"] is False
    assert "imageError" in result


def test_capture_viewport_handler_standalone_mode():
    """In standalone mode, the handler should return an error."""
    with patch("ai_chat_plugin.scene_tools.HOSTED_MODE", False):
        from ai_chat_plugin.tool_handlers import _capture_viewport_handler

        result_json = _capture_viewport_handler(1024, 768, "png")
        result = json.loads(result_json)

    assert result["ok"] is False
    assert "error" in result
```

- [ ] **Step 2: Run test to verify it fails**

Run:
```bash
.\pyvenv\Scripts\python.exe -m pytest plugins/ai_chat_plugin/tests/test_capture_tool.py -v
```
Expected: FAIL — `_capture_viewport_handler` does not exist yet.

- [ ] **Step 3: Add handler and Pydantic model to tool_handlers.py**

In `plugins/ai_chat_plugin/tool_handlers.py`, add after `_ExecuteActionParams` class (around line 42):

```python
class _CaptureViewportParams(BaseModel):
    width: int = Field(default=1024, description="Screenshot width in pixels (default 1024)")
    height: int = Field(default=768, description="Screenshot height in pixels (default 768)")
    format: str = Field(default="png", description="Image format: png or jpeg (default png)")
```

Add the handler function after `_execute_action_handler` (around line 98):

```python
# Thread-local storage for pending attachments (set by tool, read by worker)
import threading

_pending_attachments_local = threading.local()


def get_pending_attachments() -> list[dict]:
    """Retrieve and clear pending attachments set by tool handlers."""
    attachments = getattr(_pending_attachments_local, "items", [])
    _pending_attachments_local.items = []
    return attachments


def _capture_viewport_handler(width: int, height: int, fmt: str) -> str:
    """Handler for the capture_viewport tool."""
    if not scene_tools.HOSTED_MODE:
        return json.dumps({"ok": False, "error": "Not in hosted mode — pywrapper unavailable"})

    try:
        result = scene_tools.execute_action(
            "scene", "capture_viewport",
            {"width": width, "height": height, "format": fmt},
        )
    except Exception:
        return json.dumps({
            "ok": False,
            "error": f"Capture failed: {traceback.format_exc(limit=3)}",
        })

    # Extract image for BlobAttachment (not returned in tool result)
    image_b64 = result.pop("image", None)
    image_attached = False

    if image_b64 and isinstance(image_b64, str):
        mime_type = "image/jpeg" if fmt == "jpeg" else "image/png"
        blob = {
            "type": "blob",
            "data": image_b64,
            "mimeType": mime_type,
            "displayName": f"viewport.{fmt}",
        }
        if not hasattr(_pending_attachments_local, "items"):
            _pending_attachments_local.items = []
        _pending_attachments_local.items.append(blob)
        image_attached = True

    # Build metadata-only return value
    metadata = result.get("metadata", {})
    tool_return = {
        **metadata,
        "imageAttached": image_attached,
    }

    if not image_attached:
        image_error = result.get("imageError")
        if image_error:
            tool_return["imageError"] = image_error

    tool_return["note"] = (
        "Screenshot captured. Visible shapes and their screen positions are listed above."
        if image_attached
        else "Screenshot capture failed. Only metadata is available."
    )

    return json.dumps(tool_return, default=str)
```

- [ ] **Step 4: Register the tool in build_tools()**

In the `build_tools()` function, add after the `execute_action` tool definition (around line 136):

```python
    @define_tool(
        description=(
            "Capture the current 3D viewport as a screenshot with scene metadata. "
            "Returns camera position, visible shapes with screen bounding boxes, "
            "selections, and labels. The image is automatically attached so you can "
            "see what the user sees. Use this to understand the current view."
        ),
    )
    def capture_viewport(params: _CaptureViewportParams) -> str:
        return _capture_viewport_handler(params.width, params.height, params.format)

    return [describe_module, describe_action, execute_action, capture_viewport]
```

- [ ] **Step 5: Run tests to verify they pass**

Run:
```bash
.\pyvenv\Scripts\python.exe -m pytest plugins/ai_chat_plugin/tests/test_capture_tool.py -v
```
Expected: ALL PASS

- [ ] **Step 6: Commit**

```bash
git add plugins/ai_chat_plugin/tool_handlers.py
git add plugins/ai_chat_plugin/tests/test_capture_tool.py
git commit -m "feat(ai-chat): add capture_viewport tool handler

Calls C++ capture_viewport action, extracts base64 image for
BlobAttachment delivery, returns metadata as tool result.
Uses thread-local storage for pending attachments."
```

---

### Task 8: CopilotWorker Attachment Support

**Files:**
- Modify: `plugins/ai_chat_plugin/copilot_worker.py`

- [ ] **Step 1: Change queue type to carry attachments**

The current queue is `asyncio.Queue[str | None]` carrying just the message text. Change it to carry a tuple of `(text, attachments)`:

In `__init__` (around line 65), change:
```python
        self._queue: asyncio.Queue[str | None] | None = None
```
to:
```python
        self._queue: asyncio.Queue[tuple[str, list[dict]] | None] | None = None
```

- [ ] **Step 2: Update queueMessage to accept attachments**

Change `queueMessage` (around line 76):

```python
    @Slot(str, list)
    def queueMessage(self, text: str, attachments: list | None = None) -> None:
        """Enqueue a user message with optional attachments from the main thread."""
        if self._loop is not None and self._queue is not None:
            msg = (text, attachments or [])
            self._loop.call_soon_threadsafe(self._queue.put_nowait, msg)
```

- [ ] **Step 3: Update send loop to pass attachments**

In `_main()` (around line 186-190), change:

```python
                    while not self._shutdown:
                        msg = await self._queue.get()
                        if msg is None:
                            break
                        await session.send(msg)
```

to:

```python
                    while not self._shutdown:
                        item = await self._queue.get()
                        if item is None:
                            break
                        text, attachments = item

                        # Collect any pending attachments from tool handlers
                        from ai_chat_plugin.tool_handlers import get_pending_attachments
                        tool_attachments = get_pending_attachments()
                        all_attachments = attachments + tool_attachments

                        if all_attachments:
                            await session.send(text, attachments=all_attachments)
                        else:
                            await session.send(text)
```

- [ ] **Step 4: Update shutdown sentinel handling**

The sentinel `None` is still used for shutdown. Update `requestShutdown()` and `requestNewSession()` — they already use `self._queue.put_nowait(None)`, which still works since `None` is not a tuple.

- [ ] **Step 5: Build and basic smoke test**

Run:
```bash
.\pyvenv\Scripts\python.exe -c "from ai_chat_plugin.copilot_worker import CopilotWorker; print('import OK')"
```
Expected: `import OK`

- [ ] **Step 6: Commit**

```bash
git add plugins/ai_chat_plugin/copilot_worker.py
git commit -m "feat(ai-chat): extend CopilotWorker with BlobAttachment support

Queue now carries (text, attachments) tuples. On send, merges
user-provided attachments with tool-handler pending attachments.
Uses SDK session.send(text, attachments=[...]) for vision delivery."
```

---

### Task 9: ChatBackend captureViewport() + QML 📎 Button

**Files:**
- Modify: `plugins/ai_chat_plugin/chat_backend.py`
- Modify: `plugins/ai_chat_plugin/qml/ChatInputArea.qml`

- [ ] **Step 1: Add captureViewport to ChatBackend**

In `chat_backend.py`, add to imports:

```python
import base64
```

Add properties and signal (in the class, around line 36):

```python
    pendingAttachmentChanged = Signal()
```

Add to `__init__` (around line 48):

```python
        self._pending_attachment: dict | None = None  # BlobAttachment dict
        self._pending_thumbnail: str = ""  # base64 thumbnail for QML preview
```

Add property:

```python
    def _get_pending_thumbnail(self) -> str:
        return self._pending_thumbnail

    pendingThumbnail = Property(
        str, _get_pending_thumbnail, notify=pendingAttachmentChanged,
    )

    def _get_has_pending_attachment(self) -> bool:
        return self._pending_attachment is not None

    hasPendingAttachment = Property(
        bool, _get_has_pending_attachment, notify=pendingAttachmentChanged,
    )
```

Add Q_INVOKABLE methods:

```python
    @Slot()
    def captureViewport(self) -> None:
        """Capture viewport screenshot and store as pending attachment."""
        from ai_chat_plugin import scene_tools

        if not scene_tools.HOSTED_MODE:
            return

        try:
            result = scene_tools.execute_action(
                "scene", "capture_viewport",
                {"width": 1024, "height": 768, "captureImage": True},
            )
            image_b64 = result.get("image")
            if image_b64 and isinstance(image_b64, str):
                self._pending_attachment = {
                    "type": "blob",
                    "data": image_b64,
                    "mimeType": "image/png",
                    "displayName": "viewport.png",
                }
                # Use a small prefix for thumbnail (QML displays data: URI)
                self._pending_thumbnail = f"data:image/png;base64,{image_b64[:200]}..."
                self.pendingAttachmentChanged.emit()
        except Exception:
            pass  # Silently fail — capture is optional

    @Slot()
    def clearPendingAttachment(self) -> None:
        """Remove the pending viewport screenshot."""
        self._pending_attachment = None
        self._pending_thumbnail = ""
        self.pendingAttachmentChanged.emit()
```

- [ ] **Step 2: Update sendMessage to include pending attachment**

Modify the existing `sendMessage` method (around line 131):

```python
    @Slot(str)
    def sendMessage(self, text: str) -> None:
        """Append user message and forward to the SDK worker."""
        text = text.strip()
        if not text:
            return

        # Collect pending attachment (from 📎 button)
        attachments: list[dict] = []
        if self._pending_attachment is not None:
            attachments.append(self._pending_attachment)
            self._pending_attachment = None
            self._pending_thumbnail = ""
            self.pendingAttachmentChanged.emit()

        self._message_model.appendMessage("user", text)

        # Create an assistant placeholder row for streaming
        self._active_msg_id = str(uuid.uuid4())
        self._message_model.appendMessage(
            "assistant", "", msgId=self._active_msg_id
        )

        self._set_streaming(True)
        self._worker.queueMessage(text, attachments)
```

- [ ] **Step 3: Add 📎 button to ChatInputArea.qml**

In `plugins/ai_chat_plugin/qml/ChatInputArea.qml`, add signals and properties at the top of the root Rectangle (after `signal sendMessage`):

```qml
    signal captureViewport()
    signal clearAttachment()
    property bool hasAttachment: false
    property string attachmentThumbnail: ""
```

Add attachment preview bar above the RowLayout (between the root Rectangle and the RowLayout):

```qml
    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: PluginTheme.gapTight
        spacing: 2

        // Attachment preview (shown when a screenshot is pending)
        Rectangle {
            id: attachmentPreview
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? 32 : 0
            visible: root.hasAttachment
            color: PluginTheme.surfaceMuted
            radius: PluginTheme.radiusSmall / 2

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 4

                Text {
                    text: "📷"
                    font.pixelSize: 14
                }

                Text {
                    text: qsTr("viewport.png")
                    font.pixelSize: 11
                    color: PluginTheme.textSecondary
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Button {
                    text: "✕"
                    flat: true
                    implicitWidth: 20
                    implicitHeight: 20
                    onClicked: root.clearAttachment()

                    contentItem: Text {
                        text: "✕"
                        font.pixelSize: 11
                        color: PluginTheme.textTertiary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Item {}
                }
            }
        }

        // Input row
        RowLayout {
            id: inputLayout
            Layout.fillWidth: true
            spacing: PluginTheme.gapTight
```

**Restructure the existing content:** The current `RowLayout` with id `inputLayout` needs to be wrapped inside a `ColumnLayout`. Move the existing `anchors.fill: parent` and `anchors.margins` from the `RowLayout` to the new `ColumnLayout`.

Add the 📎 button before the ScrollView in the inputLayout:

```qml
            // Capture viewport button
            Button {
                id: captureButton

                text: "📎"
                Layout.alignment: Qt.AlignBottom

                onClicked: root.captureViewport()

                contentItem: Text {
                    text: captureButton.text
                    font.pixelSize: 16
                    color: PluginTheme.textSecondary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    implicitWidth: 36
                    implicitHeight: 36
                    radius: PluginTheme.radiusSmall / 2
                    color: captureButton.hovered
                           ? PluginTheme.surfaceMuted
                           : "transparent"

                    Behavior on color {
                        ColorAnimation { duration: PluginTheme.animFast }
                    }
                }
            }
```

- [ ] **Step 4: Wire QML signals in ChatPage.qml**

In `ChatPage.qml` (or wherever ChatInputArea is used), connect the new signals:

```qml
    ChatInputArea {
        // ... existing bindings ...
        hasAttachment: chatBackend.hasPendingAttachment
        attachmentThumbnail: chatBackend.pendingThumbnail

        onCaptureViewport: chatBackend.captureViewport()
        onClearAttachment: chatBackend.clearPendingAttachment()
    }
```

- [ ] **Step 5: Build and verify**

Run:
```bash
.\pyvenv\Scripts\python.exe -c "from ai_chat_plugin.chat_backend import ChatBackend; print('import OK')"
```
Expected: `import OK`

- [ ] **Step 6: Commit**

```bash
git add plugins/ai_chat_plugin/chat_backend.py
git add plugins/ai_chat_plugin/copilot_worker.py
git add plugins/ai_chat_plugin/qml/ChatInputArea.qml
git commit -m "feat(ai-chat): add viewport capture UI with 📎 button

ChatBackend.captureViewport() calls C++ action, stores base64
image as pending BlobAttachment. ChatInputArea shows 📎 button
and attachment preview strip. Attachment sent with next message."
```

---

### Task 10: System Prompt Update + Vision Capability Check

**Files:**
- Modify: `plugins/ai_chat_plugin/prompts/system_prompt.md`
- Modify: `plugins/ai_chat_plugin/copilot_worker.py` (vision check)

- [ ] **Step 1: Update system prompt with vision instructions**

In `plugins/ai_chat_plugin/prompts/system_prompt.md`, add a new section:

```markdown
## Vision Capabilities

You can see the 3D viewport by using the `capture_viewport` tool. When a user asks about
what they see, what's in the viewport, or needs help with a specific view:

1. Call `capture_viewport()` to get a screenshot and structured metadata
2. The screenshot will be attached to your context automatically
3. The metadata includes: camera position, visible shapes with screen bounding boxes,
   current selections, and labels
4. Use both the image and metadata to give precise, spatial answers

The user can also manually attach a viewport screenshot using the 📎 button.
When you see an attached image, describe what you observe and relate it to the metadata.

### When to use capture_viewport:
- User asks "what do I see?" or "describe the viewport"
- User asks about a specific shape's position or orientation
- You need to verify the result of a geometry operation you just performed
- User asks for help with selection or labeling

### Screen coordinate system:
- Origin (0,0) is at the **top-left** corner
- X increases rightward, Y increases downward
- screenBBox values are in pixels relative to the capture resolution
```

- [ ] **Step 2: Add vision capability check to worker**

In `copilot_worker.py`, add a helper to check if the current model supports vision. In the `_on_event` handler or in the model loading flow, store the vision capability:

Add to `__init__`:
```python
        self._model_supports_vision: bool = True  # assume true, check on model load
```

In `_fetch_models()` (if this method exists), after loading models:
```python
    # Check if current model supports vision
    for model in models:
        if model.get("id") == current_model_id:
            capabilities = model.get("capabilities", {})
            supports = capabilities.get("supports", {})
            self._model_supports_vision = supports.get("vision", False)
            break
```

This is informational — the tool should still work even without explicit vision support (the image data might be dropped by the model, but the metadata is still useful as text).

- [ ] **Step 3: Run full Python test suite**

Run:
```bash
.\pyvenv\Scripts\python.exe -m pytest plugins/ai_chat_plugin/tests/ -v
```
Expected: ALL PASS

- [ ] **Step 4: Sync plugin to build dir and manual test**

Run:
```powershell
Remove-Item -Recurse -Force build\bin\plugins\ai_chat_plugin
Copy-Item -Path "plugins\ai_chat_plugin" -Destination "build\bin\plugins\ai_chat_plugin" -Recurse -Force
```

Then launch the application, open the AI chat, and:
1. Verify the 📎 button appears in the input area
2. Click 📎 — should show "viewport.png" preview strip
3. Click ✕ to remove the attachment
4. Type a message with attachment and send — verify it goes through

- [ ] **Step 5: Commit**

```bash
git add plugins/ai_chat_plugin/prompts/system_prompt.md
git add plugins/ai_chat_plugin/copilot_worker.py
git commit -m "feat(ai-chat): update system prompt with vision capabilities

Adds instructions for capture_viewport tool usage, screen coordinate
system explanation, and when to use viewport capture. Adds model
vision capability tracking in CopilotWorker."
```

---

## End-to-End Verification Checklist

After all 3 parts are complete:

1. **C++ build:** `cmake --build build --config RelWithDebInfo --parallel 4` — SUCCESS
2. **C++ tests:** `ctest --test-dir build -C RelWithDebInfo --output-on-failure` — ALL PASS
3. **Python tests:** `.\pyvenv\Scripts\python.exe -m pytest plugins/ai_chat_plugin/tests/ -v` — ALL PASS
4. **Manual E2E:**
   - Load a STEP model in the app
   - Open AI chat panel
   - Ask "describe what you see in the viewport" → AI calls capture_viewport
   - Click 📎 → attachment preview appears → send message with attachment
   - Verify AI sees and describes the viewport image

---

## SDK Fallback Strategy

If the Copilot SDK's `BlobAttachment` does not work with certain models (e.g., model drops the image):

**Fallback A:** Embed base64 image directly in the tool return as an `image_base64` field. Some models can decode inline base64 images.

**Fallback B:** Save the PNG to a temp file and use `FileAttachment`:
```python
{"type": "file", "path": "/tmp/viewport_capture.png"}
```

**Fallback C:** Return only metadata (no image). The metadata alone provides significant context with screen bounding boxes, camera state, selections, and labels.

The current implementation uses `BlobAttachment` as the primary path, with automatic fallback to metadata-only if image capture fails or times out.
