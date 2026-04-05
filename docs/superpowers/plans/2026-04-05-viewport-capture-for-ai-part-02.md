# Viewport Capture for AI — Part 2 of 3

> Part 文件：C++ GLViewport capture mechanism (FBO readback + PNG encoding) — Tasks 4-6.
> 依赖：Part 1 完成（CaptureViewportAction 已注册并返回 metadata）。

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **Recommended models:** Use Claude Opus 4.6 for design or plan revisions. Use GPT-5.4 for implementation, debugging, verification, and code-review subtasks.

**Goal:** Add a cross-thread viewport capture mechanism to `GLViewport`/`GLViewportRenderer` that reads the FBO pixels after rendering, encodes to PNG, and returns base64 data. Then wire this into `CaptureViewportAction` so the action returns both metadata and image.

**Architecture:** Uses the existing `PendingPick`/`PendingBoxSelect` request-callback pattern. A `PendingCapture` struct is set on `GLViewport` from the main thread, consumed by `GLViewportRenderer::render()` on the render thread after pipeline execution. Pixels are read via `glReadPixels`, encoded to PNG via `stb_image_write`, base64-encoded, and delivered back to the main thread via `QMetaObject::invokeMethod` + a `std::promise`.

**Tech Stack:** C++20 · OpenGL · stb_image_write · QQuickFramebufferObject · std::promise/future

---

## File Map

| Action | Path | Responsibility |
|--------|------|----------------|
| GLViewport header | `src/app/include/opengeolab/app/gl_viewport.hpp` | Add `PendingCapture`, `requestCapture()`, `captureCompleted` signal |
| GLViewport impl | `src/app/src/gl_viewport.cpp` | `requestCapture()` sets pending + triggers update |
| Renderer impl | `src/app/src/gl_viewport_renderer.cpp` | Execute capture in `render()`, `glReadPixels` + PNG encode |
| Base64 utility | `src/libs/core/include/opengeolab/core/base64.hpp` | Standalone base64 encoder (header-only) |
| Action impl | `src/libs/scene/src/capture_viewport_action.cpp` | Wire capture request through ViewportState |
| ViewportState | `src/libs/scene/include/opengeolab/scene/viewport_state.hpp` | Add capture request/result queuing |
| ViewportState impl | `src/libs/scene/src/viewport_state.cpp` | Capture request implementation |

---

### Task 4: Add Capture Infrastructure to ViewportState

**Files:**
- Modify: `src/libs/scene/include/opengeolab/scene/viewport_state.hpp`
- Modify: `src/libs/scene/src/viewport_state.cpp`

The capture crosses threads (action thread → render thread → action thread). We use `ViewportState` as the shared coordination point, similar to how `PendingPickArea` works.

- [ ] **Step 1: Define CaptureRequest and CaptureResult types**

Add to `src/libs/scene/include/opengeolab/scene/viewport_state.hpp`, after the `PendingPickArea` struct (around line 38):

```cpp
/// @brief Pending viewport capture request (async, consumed by renderer).
struct PendingCapture {
    int width{1024};   ///< Desired capture width in pixels
    int height{768};   ///< Desired capture height in pixels
    /// Shared promise to deliver the base64 PNG result back to the requester.
    std::shared_ptr<std::promise<std::string>> promise;
};
```

Add to the includes at the top of the file:

```cpp
#include <future>
#include <memory>
#include <string>
```

- [ ] **Step 2: Add capture request/consume methods to ViewportState**

In the `ViewportState` class (after `consumePickArea()`, around line 70), add:

```cpp
    /// @brief Queue a viewport capture request for the renderer.
    void requestCapture(PendingCapture request);

    /// @brief Consume and return the pending capture (if any).
    [[nodiscard]] std::optional<PendingCapture> consumeCapture();

    Kangaroo::Util::Signal<> captureRequested; ///< Fired after capture queued
```

Add to private members (after `m_pendingPickArea`, around line 79):

```cpp
    std::optional<PendingCapture> m_pendingCapture;
```

- [ ] **Step 3: Implement requestCapture/consumeCapture in viewport_state.cpp**

View `src/libs/scene/src/viewport_state.cpp` to find the existing `requestPickArea`/`consumePickArea` pattern, then add analogous methods:

```cpp
void ViewportState::requestCapture(PendingCapture request) {
    {
        std::lock_guard lock(m_mutex);
        m_pendingCapture = std::move(request);
    }
    captureRequested.emit();
}

std::optional<PendingCapture> ViewportState::consumeCapture() {
    std::lock_guard lock(m_mutex);
    auto result = std::move(m_pendingCapture);
    m_pendingCapture.reset();
    return result;
}
```

- [ ] **Step 4: Build to verify compilation**

Run:
```bash
cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 4
```
Expected: BUILD SUCCESS

- [ ] **Step 5: Commit**

```bash
git add src/libs/scene/include/opengeolab/scene/viewport_state.hpp
git add src/libs/scene/src/viewport_state.cpp
git commit -m "feat(scene): add PendingCapture request/consume to ViewportState

Adds async capture request queuing similar to PendingPickArea.
The renderer consumes the request after rendering and delivers
the base64 PNG result via std::promise."
```

---

### Task 5: Add Base64 Encoder Utility + GLViewportRenderer Capture

**Files:**
- Create: `src/libs/core/include/opengeolab/core/base64.hpp`
- Modify: `src/app/src/gl_viewport_renderer.cpp`
- Modify: `src/app/include/opengeolab/app/gl_viewport.hpp`
- Modify: `src/app/src/gl_viewport.cpp`

- [ ] **Step 1: Create base64 header-only encoder**

Create `src/libs/core/include/opengeolab/core/base64.hpp`:

```cpp
/**
 * @file base64.hpp
 * @brief Minimal base64 encoder for binary data (e.g. PNG images).
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace OpenGeoLab::Core {

/**
 * @brief Encode binary data to a base64 string.
 * @param data Input bytes to encode.
 * @return Base64-encoded string (no line breaks).
 */
inline std::string base64Encode(std::span<const uint8_t> data) {
    static constexpr const char* K_TABLE =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string result;
    result.reserve(((data.size() + 2) / 3) * 4);

    size_t i = 0;
    while(i + 2 < data.size()) {
        uint32_t triplet = (static_cast<uint32_t>(data[i]) << 16) |
                           (static_cast<uint32_t>(data[i + 1]) << 8) |
                           static_cast<uint32_t>(data[i + 2]);
        result += K_TABLE[(triplet >> 18) & 0x3F];
        result += K_TABLE[(triplet >> 12) & 0x3F];
        result += K_TABLE[(triplet >> 6) & 0x3F];
        result += K_TABLE[triplet & 0x3F];
        i += 3;
    }

    if(i + 1 == data.size()) {
        uint32_t val = static_cast<uint32_t>(data[i]) << 16;
        result += K_TABLE[(val >> 18) & 0x3F];
        result += K_TABLE[(val >> 12) & 0x3F];
        result += '=';
        result += '=';
    } else if(i + 2 == data.size()) {
        uint32_t val = (static_cast<uint32_t>(data[i]) << 16) |
                       (static_cast<uint32_t>(data[i + 1]) << 8);
        result += K_TABLE[(val >> 18) & 0x3F];
        result += K_TABLE[(val >> 12) & 0x3F];
        result += K_TABLE[(val >> 6) & 0x3F];
        result += '=';
    }

    return result;
}

} // namespace OpenGeoLab::Core
```

- [ ] **Step 2: Add base64 header to core CMakeLists.txt**

Find `src/libs/core/CMakeLists.txt` and add `include/opengeolab/core/base64.hpp` to the public headers list.

- [ ] **Step 3: Add capture handling to GLViewportRenderer::render()**

In `src/app/src/gl_viewport_renderer.cpp`, add includes at the top:

```cpp
#include <opengeolab/core/base64.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
// If stb_image_write.h is already included elsewhere without the implementation
// macro, use a separate .cpp. Check if stb is already used in the project.
// If stb_image_write is not available via CPM, use QImage::save() instead.
#include <stb_image_write.h>

#include <future>
#include <vector>
```

**Important:** Check if `stb_image_write.h` is already available in the project. The CPM dependency list includes `stb` (for `stb_image.h`). If `stb_image_write.h` is available, use it. Otherwise, use Qt's `QImage` for PNG encoding:

```cpp
#include <QImage>
#include <QBuffer>
#include <QByteArray>
```

In the `render()` method, after `m_pipeline.render(m_frameState)` and after the existing pick dispatches, add capture handling:

```cpp
    // ── Viewport capture ──
    if(auto capture = m_viewportState->consumeCapture()) {
        executeCaptureRequest(*capture);
    }
```

Add the helper method to the renderer class:

```cpp
void GLViewportRenderer::executeCaptureRequest(const Scene::PendingCapture& capture) {
    // Read pixels from the current FBO (already rendered)
    const int w = m_frameState.viewportWidth;
    const int h = m_frameState.viewportHeight;

    std::vector<uint8_t> pixels(static_cast<size_t>(w) * h * 4);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // OpenGL reads bottom-up; flip vertically for PNG (top-down)
    const size_t row_bytes = static_cast<size_t>(w) * 4;
    std::vector<uint8_t> flipped(pixels.size());
    for(int row = 0; row < h; ++row) {
        std::memcpy(flipped.data() + row * row_bytes,
                    pixels.data() + (h - 1 - row) * row_bytes,
                    row_bytes);
    }

    // Encode to PNG using stb_image_write (writes to memory)
    std::vector<uint8_t> png_data;
    auto write_func = [](void* context, void* data, int size) {
        auto* out = static_cast<std::vector<uint8_t>*>(context);
        auto* bytes = static_cast<uint8_t*>(data);
        out->insert(out->end(), bytes, bytes + size);
    };

    stbi_write_png_to_func(write_func, &png_data, w, h, 4, flipped.data(),
                           static_cast<int>(row_bytes));

    // Base64 encode
    std::string base64 = Core::base64Encode(png_data);

    // Deliver result via promise
    if(capture.promise) {
        capture.promise->set_value(std::move(base64));
    }
}
```

**Alternative if using QImage instead of stb:**

```cpp
void GLViewportRenderer::executeCaptureRequest(const Scene::PendingCapture& capture) {
    const int w = m_frameState.viewportWidth;
    const int h = m_frameState.viewportHeight;

    std::vector<uint8_t> pixels(static_cast<size_t>(w) * h * 4);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // QImage from raw pixels — OpenGL is bottom-up, QImage is top-down
    QImage image(pixels.data(), w, h, QImage::Format_RGBA8888);
    QImage flipped = image.mirrored(false, true); // flip vertically

    // Encode to PNG in memory
    QByteArray png_bytes;
    QBuffer buffer(&png_bytes);
    buffer.open(QIODevice::WriteOnly);
    flipped.save(&buffer, "PNG");

    // Base64 encode
    std::string base64 = png_bytes.toBase64().toStdString();

    if(capture.promise) {
        capture.promise->set_value(std::move(base64));
    }
}
```

The QImage approach is simpler and avoids the stb dependency question. **Prefer QImage** since Qt is already a dependency.

- [ ] **Step 4: Wire ViewportState into GLViewportRenderer::synchronize()**

In `gl_viewport_renderer.cpp`, the `synchronize()` method already reads from the viewport via `GLViewport*`. Ensure the renderer stores a pointer to `ViewportState` so it can call `consumeCapture()` during `render()`.

If `GLViewportRenderer` doesn't already hold a `ViewportState*`, add it:

In `gl_viewport_renderer.cpp` (or its private header), add member:
```cpp
Scene::ViewportState* m_viewportState{nullptr};
```

In `synchronize()`:
```cpp
m_viewportState = &viewport->sceneGraph()->viewportState();
```

The `render()` method already has access after synchronize copies state.

- [ ] **Step 5: Build and test manually**

Run:
```bash
cmake --build build --config RelWithDebInfo --parallel 4
```
Expected: BUILD SUCCESS

Note: Full integration testing requires a running Qt Quick window. Manual verification: run the app, call the action, check base64 output decodes to a valid PNG.

- [ ] **Step 6: Commit**

```bash
git add src/libs/core/include/opengeolab/core/base64.hpp
git add src/libs/core/CMakeLists.txt
git add src/app/src/gl_viewport_renderer.cpp
git add src/app/include/opengeolab/app/gl_viewport.hpp
git add src/app/src/gl_viewport.cpp
git commit -m "feat(render): add viewport capture via FBO readback + PNG encoding

GLViewportRenderer reads pixels after rendering, encodes to PNG
via QImage, and delivers base64 data via std::promise through
ViewportState. Adds Core::base64Encode utility header."
```

---

### Task 6: Wire Capture Into CaptureViewportAction

**Files:**
- Modify: `src/libs/scene/src/capture_viewport_action.cpp`

- [ ] **Step 1: Add capture request to execute()**

In `capture_viewport_action.cpp`, after collecting metadata but before the final return, add the capture request:

```cpp
    // ── Request image capture from render thread ──
    auto promise = std::make_shared<std::promise<std::string>>();
    auto future = promise->get_future();

    Scene::PendingCapture capture_req;
    capture_req.width = width;
    capture_req.height = height;
    capture_req.promise = promise;

    // This is non-const, so we need mutable access to viewportState.
    // The action constructor receives const SceneGraph&, but viewportState()
    // is a non-const method. We need to adjust the constructor.
```

**Important design note:** The action currently takes `const SceneGraph&`, but `requestCapture()` mutates `ViewportState`. Two options:

**Option A (recommended):** Change the constructor to take non-const `SceneGraph&`:
```cpp
explicit CaptureViewportAction(SceneGraph& graph);
```
And update the registration in `scene_module.cpp`:
```cpp
registerAction<CaptureViewportAction>(std::ref(m_sceneGraph));
```

**Option B:** Keep the const ref and have `ViewportState::requestCapture()` be const-qualified with mutable internal state (less clean).

Go with Option A. Update header, implementation, and registration.

- [ ] **Step 2: Update header for non-const SceneGraph ref**

In `capture_viewport_action.hpp`:

```cpp
class OPENGEOLAB_SCENE_EXPORT CaptureViewportAction final : public Core::IAction {
public:
    explicit CaptureViewportAction(SceneGraph& graph);
    // ... rest unchanged
private:
    SceneGraph& m_graph;
};
```

Update `scene_module.cpp` registration:
```cpp
registerAction<CaptureViewportAction>(std::ref(m_sceneGraph));
```

- [ ] **Step 3: Add image capture with timeout to execute()**

Add to execute(), after metadata collection:

```cpp
    // ── Request image capture ──
    auto promise = std::make_shared<std::promise<std::string>>();
    auto future = promise->get_future();

    PendingCapture capture_req;
    capture_req.width = width;
    capture_req.height = height;
    capture_req.promise = promise;
    m_graph.viewportState().requestCapture(std::move(capture_req));

    // Wait for render thread to complete capture (timeout 5 seconds)
    if(future.wait_for(std::chrono::seconds(5)) == std::future_status::ready) {
        result["image"] = future.get();
    } else {
        result["image"] = nullptr;
        result["imageError"] = "Capture timed out — render thread did not respond within 5s.";
    }
```

Add includes:
```cpp
#include <chrono>
#include <future>
#include <memory>
```

- [ ] **Step 4: Build and verify**

Run:
```bash
cmake --build build --config RelWithDebInfo --parallel 4
```
Expected: BUILD SUCCESS

- [ ] **Step 5: Run scene tests**

Run:
```bash
ctest --test-dir build -C RelWithDebInfo -R opengeolab_scene_test --output-on-failure
```

Expected: Tests pass. The capture will timeout in unit tests (no render thread running), which is expected. The test should handle this:

Update the test in `capture_viewport_action_test.cpp` — the `execute()` will now block for 5 seconds waiting for a render thread that doesn't exist. **Add a note or adjust:** For unit tests without a render loop, the action should return `imageError` after timeout. This is acceptable behavior.

Alternatively, add a parameter to skip image capture for testing:

```json
{ "captureImage": false }
```

Add this parameter to `describe()` and `execute()`:
```cpp
const bool captureImage = param.value("captureImage", true);
```

Only request capture if `captureImage` is true. In unit tests, pass `{"captureImage": false}` to avoid the 5-second timeout.

- [ ] **Step 6: Update tests to skip image capture**

In `capture_viewport_action_test.cpp`, update all existing tests to include `{"captureImage", false}` in the param:

```cpp
TEST_CASE("execute on empty scene returns valid metadata") {
    SceneGraph graph;
    CaptureViewportAction action(graph);

    auto result = action.execute({{"captureImage", false}}, nullptr);
    // ... rest unchanged
```

Add a dedicated test for the image timeout case:

```cpp
TEST_CASE("execute with captureImage=true times out without renderer") {
    SceneGraph graph;
    CaptureViewportAction action(graph);

    // This will timeout after 5 seconds since no render thread is running
    auto result = action.execute({{"captureImage", true}}, nullptr);

    CHECK(result["ok"] == true);
    CHECK(result["image"].is_null());
    CHECK(result.contains("imageError"));
}
```

**Warning:** This test takes 5 seconds. Consider using a shorter timeout for testing or marking it as a slow test.

- [ ] **Step 7: Commit**

```bash
git add src/libs/scene/include/opengeolab/scene/capture_viewport_action.hpp
git add src/libs/scene/src/capture_viewport_action.cpp
git add src/libs/scene/src/scene_module.cpp
git add src/libs/scene/test/capture_viewport_action_test.cpp
git commit -m "feat(scene): wire image capture into CaptureViewportAction

Action requests FBO capture via ViewportState, waits up to 5s
for render thread response. Returns base64 PNG in 'image' field.
Adds 'captureImage' parameter to skip image capture in tests."
```

---

## Cross-Thread Data Flow Summary

```
Main/Worker Thread                 Render Thread
─────────────────                 ─────────────
CaptureViewportAction::execute()
  │
  ├─ Collect metadata (camera, shapes, selections, labels)
  │
  ├─ Create std::promise<string>
  │
  ├─ ViewportState::requestCapture(PendingCapture{w,h,promise})
  │      │
  │      ├─ captureRequested signal ──────► GLViewport::update()
  │      │                                       │
  │      │                         GLViewportRenderer::synchronize()
  │      │                           reads PendingCapture via consumeCapture()
  │      │                                       │
  │      │                         GLViewportRenderer::render()
  │      │                           m_pipeline.render(...)
  │      │                           glReadPixels(...)
  │      │                           QImage → PNG → base64
  │      │                           promise->set_value(base64)
  │      │                                       │
  ├─ future.wait_for(5s) ◄──────────────────────┘
  │
  ├─ Assemble result JSON
  │
  └─ Return {ok, action, metadata, image}
```

**Key constraint:** `synchronize()` runs on the render thread but has access to `GLViewport*` (main thread object) through `QQuickFramebufferObject::Renderer`. The `consumeCapture()` call happens here. The actual `glReadPixels` happens in `render()` which is also on the render thread.

**Alternative flow:** If `synchronize()` happens before `render()` and the capture request arrives after `synchronize()`, it will be picked up on the *next* frame. This is fine — the latency is one frame (~16ms at 60fps).
