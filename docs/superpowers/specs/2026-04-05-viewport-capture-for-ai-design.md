# Viewport Capture for AI — Design Spec

> **Date:** 2026-04-05
> **Scope:** C++ capture action + Python tool handler + QML 用户入口 + SDK vision 集成
> **Branch:** `dev/new_6_dev`

---

## 1. 目标

让 AI 模型能够"看到"当前 3D 视口的渲染内容，结合结构化元数据理解场景。
同时支持用户手动将视口截图附加到对话消息中。

### 核心原则

- **干净图像 + 丰富元数据**：截图保持视觉干净（包含用户手动添加的 label），
  结构化 JSON 元数据提供实体级信息
- **双触发**：AI 通过 Tool Calling 主动截图，用户通过 UI 按钮手动附加
- **可配置分辨率**：默认 1024×768，AI 可按需指定

---

## 2. 数据流

```
┌─ 触发源 ──────────────────────────────────────────────────────────┐
│                                                                    │
│  AI Tool Call                      用户点击 📎 按钮               │
│  capture_viewport(w,h)             captureAndAttach()              │
│       │                                  │                         │
│       ▼                                  ▼                         │
│  ┌──────────────────────────────────────────────┐                  │
│  │  Python tool_handler / ChatBackend            │                  │
│  │  → execute_action("scene","capture_viewport") │                  │
│  └──────────────┬───────────────────────────────┘                  │
│                 │                                                   │
│                 ▼                                                   │
│  ┌──────────────────────────────────────┐                          │
│  │  C++ CaptureViewportAction           │                          │
│  │  1. 通知 render thread 截图          │                          │
│  │  2. 收集 scene metadata              │                          │
│  │  3. 等待截图完成                     │                          │
│  │  4. 返回 base64 PNG + metadata JSON  │                          │
│  └──────────────┬───────────────────────┘                          │
│                 │                                                   │
│                 ▼                                                   │
│  ┌──────────────────────────────────────┐                          │
│  │  Python 接收结果                     │                          │
│  │  ├─ AI 路径: image → BlobAttachment  │                          │
│  │  │           metadata → tool return  │                          │
│  │  └─ 用户路径: image → 消息附件显示   │                          │
│  └──────────────────────────────────────┘                          │
└────────────────────────────────────────────────────────────────────┘
```

---

## 3. C++ Action: `scene.capture_viewport`

### 3.1 位置

新增 `CaptureViewportAction` 到 **scene 模块**（因为需要访问 SceneGraph、
SelectionState、LabelManager 和渲染管线）。

### 3.2 请求参数

```json
{
  "module": "scene",
  "action": "capture_viewport",
  "params": {
    "width": 1024,
    "height": 768,
    "format": "png",
    "includeMetadata": true
  }
}
```

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `width` | int | 1024 | 截图宽度（像素） |
| `height` | int | 768 | 截图高度（像素） |
| `format` | string | `"png"` | 图片格式（`"png"` 或 `"jpeg"`） |
| `includeMetadata` | bool | true | 是否收集元数据 |

### 3.3 返回值

```json
{
  "ok": true,
  "action": "capture_viewport",
  "image": "<base64-encoded PNG>",
  "metadata": {
    "viewport": { "width": 1024, "height": 768 },
    "camera": {
      "eye": [x, y, z],
      "target": [x, y, z],
      "up": [x, y, z]
    },
    "visibleShapes": [
      {
        "shapeId": 1,
        "name": "Box_1",
        "screenBBox": { "x": 100, "y": 50, "w": 400, "h": 300 },
        "subEntities": {
          "solids": 1,
          "faces": 6,
          "edges": 12,
          "vertices": 8
        }
      }
    ],
    "selections": [
      { "shapeId": 1, "type": "GeoFace", "localId": 3 }
    ],
    "labels": [
      {
        "text": "F:3",
        "entity": { "shapeId": 1, "type": "GeoFace", "localId": 3 }
      }
    ],
    "hover": { "shapeId": 1, "type": "GeoEdge", "localId": 5 }
  }
}
```

### 3.4 实现策略

**挑战：** 渲染发生在 render thread（`QQuickFramebufferObject::Renderer`），
而 Action 执行在 main thread 或 worker thread。需要跨线程同步。

**方案：请求-回调模式**

1. `CaptureViewportAction::execute()` 设置一个 capture 请求（目标分辨率 + 格式）
2. 通过信号通知 `GLViewport` 有截图请求
3. `GLViewportRenderer::render()` 在下一帧渲染完成后：
   - 如果请求了特定分辨率且与当前 FBO 不同，使用离屏 FBO 重新渲染
   - 调用 `glReadPixels` 读取颜色缓冲
   - 编码为 PNG（使用 `stb_image_write` 或 Qt 的 `QImage::save`）
   - 通过回调/promise 返回 base64 数据
4. Action 等待回调完成（带超时），收集 metadata，组装返回 JSON

**关键依赖：**
- `GLViewport` / `GLViewportRenderer`：截图执行
- `SceneGraph`：遍历可见节点
- `SelectionState`：当前选中实体
- `LabelManager`：已有标签
- `GeometryModule`（ShapeStore）：子实体计数
- 渲染管线的 MVP 矩阵：计算屏幕包围盒

### 3.5 屏幕包围盒计算

对每个可见 shape，使用其世界空间 AABB 的 8 个顶点：

```cpp
// 伪代码
auto corners = shape.worldBounds().corners();  // 8 个 vec3
float minX = INF, minY = INF, maxX = -INF, maxY = -INF;
for (auto& c : corners) {
    auto clip = mvp * vec4(c, 1.0);
    if (clip.w <= 0) continue;  // behind camera
    auto ndc = vec3(clip) / clip.w;
    float sx = (ndc.x * 0.5 + 0.5) * viewportWidth;
    float sy = (1.0 - (ndc.y * 0.5 + 0.5)) * viewportHeight;  // flip Y
    minX = min(minX, sx); minY = min(minY, sy);
    maxX = max(maxX, sx); maxY = max(maxY, sy);
}
// screenBBox = { x: minX, y: minY, w: maxX-minX, h: maxY-minY }
```

---

## 4. Python Tool Handler: `capture_viewport`

### 4.1 工具定义

在 `tool_handlers.py` 新增第 4 个工具：

```python
class _CaptureViewportParams(BaseModel):
    width: int = Field(default=1024, description="Screenshot width in pixels")
    height: int = Field(default=768, description="Screenshot height in pixels")
    format: str = Field(default="png", description="Image format: png or jpeg")

@define_tool(description="Capture the current 3D viewport as an image. "
             "Returns structured metadata about visible shapes, selections, "
             "and labels. The image is automatically attached to your next response "
             "so you can see what the user sees.")
def capture_viewport(params: _CaptureViewportParams) -> str:
    ...
```

### 4.2 BlobAttachment 集成

**关键设计决策：** Tool handler 不能直接往 session 发送 attachment。
需要通过信号链将 base64 图片传回 `CopilotWorker`，在发送下一条消息时附加。

**实现方案：pending attachment 队列**

```
capture_viewport tool handler
  → 调用 C++ action 获取 image + metadata
  → 将 base64 image 存入 CopilotWorker._pending_attachments 列表
  → 返回 metadata JSON 作为工具结果
  → SDK 将 metadata 发给模型
  → 模型生成下一轮回复时，SDK 自动附带 pending attachments
```

**备选方案：** 如果 SDK 不支持在 tool result 中附加 attachment，
则将 base64 image 内嵌到 tool 返回值的 JSON 中（作为 `image_base64` 字段），
让模型自行将其理解为视觉输入。但这取决于模型能力。

**推荐实现：** 先尝试 SDK 的 BlobAttachment 机制。如果 tool execution 上下文
不支持 attachment，则 fallback 到在 tool 返回值中内嵌 base64。

### 4.3 工具返回值（metadata 部分）

```json
{
  "_request": "scene.capture_viewport({...})",
  "viewport": { "width": 1024, "height": 768 },
  "camera": { "eye": [...], "target": [...], "up": [...] },
  "visibleShapes": [...],
  "selections": [...],
  "labels": [...],
  "imageAttached": true,
  "note": "Screenshot captured. Visible shapes and their screen positions are listed above."
}
```

---

## 5. QML 用户手动截图入口

### 5.1 ChatInputArea 增加 📎 按钮

在 ChatInputArea.qml 的发送按钮旁增加一个"附加视口截图"按钮：

```
┌─────────────────────────────────────────────┐
│ [📎]  输入消息...                    [发送] │
└─────────────────────────────────────────────┘
```

点击 📎 触发 `ChatBackend.captureViewport()` → 调用 C++ action →
截图作为待发送附件，显示为消息输入区的缩略图预览。

### 5.2 附件预览

在输入区上方显示小缩略图，用户可移除：

```
┌─────────────────────────────────────────────┐
│ [viewport.png ×]                            │
│ [📎]  描述一下这个模型...            [发送] │
└─────────────────────────────────────────────┘
```

### 5.3 发送时附加

用户点击发送 → `CopilotWorker.sendMessage(text, attachments=[blob])` →
SDK `session.send(text, attachments=[...])` 发送文本 + 图片。

---

## 6. CopilotWorker / ChatBackend 改动

### 6.1 CopilotWorker

- 新增 `_pending_attachments: list[dict]` 队列
- `sendMessage` 信号增加可选 `attachments` 参数
- `_run_session()` 中 `session.send()` 调用时带上 attachments
- Tool handler 可向 `_pending_attachments` 追加图片

### 6.2 ChatBackend

- 新增 `captureViewport()` Q_INVOKABLE 方法
- 新增 `pendingAttachment` / `pendingAttachmentChanged` 属性/信号
- 截图完成后设置 pending attachment，QML 显示预览
- `sendMessage()` 时带上 pending attachment 并清空

### 6.3 sendMessage 签名变更

```python
# CopilotWorker
sendRequested = Signal(str, list)  # text, attachments

# ChatBackend
@Slot(str)
def sendMessage(self, text: str):
    attachments = self._pending_attachments.copy()
    self._pending_attachments.clear()
    self._worker.sendRequested.emit(text, attachments)
```

---

## 7. SDK Vision 能力检查

在发送图片前检查当前模型是否支持 vision：

```python
model_info = next((m for m in self._models if m["id"] == self._current_model), None)
if model_info and not model_info.get("vision", False):
    # 降级：不发送图片，在 metadata 中说明
    return {"error": "Current model does not support vision. Switch to a vision-capable model."}
```

UI 层可在 ModelSelectorBar 中标注哪些模型支持视觉（如 👁️ 图标）。

---

## 8. 分辨率与性能

| 分辨率 | 估计 PNG 大小 | Token 成本 | 适用场景 |
|--------|-------------|-----------|---------|
| 512×384 | ~50KB | 低 | 快速概览 |
| 1024×768 | ~200KB | 中 | 默认，够用 |
| 2048×1536 | ~800KB | 高 | 需要细节 |

- 默认 1024×768，AI 可通过参数指定
- JPEG 可用于降低体积（quality=85 约为 PNG 的 1/3）
- 如果超出模型的 `max_prompt_image_size`，自动降级分辨率

---

## 9. 文件清单

### 新增文件

| 文件 | 说明 |
|------|------|
| `src/libs/scene/include/opengeolab/scene/capture_viewport_action.hpp` | Action 头文件 |
| `src/libs/scene/src/capture_viewport_action.cpp` | Action 实现 |

### 修改文件

| 文件 | 改动 |
|------|------|
| `src/libs/scene/CMakeLists.txt` | 注册新 Action 源文件 |
| `src/libs/scene/src/scene_module.cpp` | 注册 CaptureViewportAction |
| `src/app/include/opengeolab/app/gl_viewport.hpp` | 添加截图请求接口 |
| `src/app/src/gl_viewport.cpp` | 截图请求处理 |
| `src/app/src/gl_viewport_renderer.cpp` | render 线程截图执行 |
| `plugins/ai_chat_plugin/tool_handlers.py` | 新增 capture_viewport 工具 |
| `plugins/ai_chat_plugin/copilot_worker.py` | pending_attachments + sendMessage 改造 |
| `plugins/ai_chat_plugin/chat_backend.py` | captureViewport() + pendingAttachment |
| `plugins/ai_chat_plugin/qml/ChatInputArea.qml` | 📎按钮 + 附件预览 |
| `plugins/ai_chat_plugin/prompts/system_prompt.md` | 视觉能力说明 |

### 测试

| 文件 | 说明 |
|------|------|
| `src/libs/scene/test/capture_viewport_test.cpp` | C++ Action 单元测试（metadata 组装） |
| `plugins/ai_chat_plugin/tests/test_capture_viewport.py` | Python tool handler 测试 |

---

## 10. 不在范围内

- 视频/GIF 录制
- 实时视觉流（每帧发送）
- 自动标注密集实体（用户手动 add_label 代替）
- 多视口截图
- 3D 模型文件导出给 AI
