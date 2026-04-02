# Render Module Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the `opengeolab_render` library and App integration layer to provide 3D OpenGL rendering with 4-pass pipeline (opaque, wireframe, highlight, selection), GPU picking, and interactive camera in a QML viewport.

**Architecture:** Pure-GL render library (`opengeolab_render`) with zero Qt dependency, using glad for GL function loading and glm for math. App integration layer extends `src/app/` with `GLViewport` (QQuickFramebufferObject), `CameraState`, and `TrackballController`. Data flows from SceneGraph → GpuBufferManager → 4 RenderPasses → PickFbo → PickResolver.

**Tech Stack:** C++20, glad 2.x (CPM), glm 1.0.1 (existing), OpenGL 3.3 Core, Qt 6.9 (app layer only), doctest (testing)

**Spec:** `docs/superpowers/specs/2026-03-30-scene-render-design.md`

---

## File Structure

### New: `src/libs/render/`

```
src/libs/render/
├── CMakeLists.txt
├── include/opengeolab/render/
│   ├── frame_state.hpp             # FrameState struct
│   ├── pick_mask.hpp               # PickMode, PickMask enums
│   ├── pick_result.hpp             # PickResult struct
│   ├── render_pipeline.hpp         # RenderPipeline top-level class
│   └── batch_utils.hpp             # IndexedBatch, ArrayBatch, glMultiDraw
├── src/
│   ├── render_pipeline.cpp
│   ├── core/
│   │   ├── shader_program.hpp      # ShaderProgram compile/link
│   │   ├── shader_program.cpp
│   │   ├── gpu_buffer_manager.hpp  # VAO/VBO/IBO upload and bind
│   │   ├── gpu_buffer_manager.cpp
│   │   ├── pick_fbo.hpp            # Off-screen pick FBO
│   │   └── pick_fbo.cpp
│   ├── pass/
│   │   ├── render_pass_base.hpp    # Abstract pass base
│   │   ├── opaque_pass.hpp
│   │   ├── opaque_pass.cpp
│   │   ├── wireframe_pass.hpp
│   │   ├── wireframe_pass.cpp
│   │   ├── highlight_pass.hpp
│   │   ├── highlight_pass.cpp
│   │   ├── selection_pass.hpp
│   │   └── selection_pass.cpp
│   ├── pick_resolver.hpp
│   ├── pick_resolver.cpp
│   └── batch_utils.cpp
└── test/
    ├── pick_resolver_test.cpp
    └── batch_utils_test.cpp
```

### Modified: `src/app/`

```
src/app/
├── include/opengeolab/app/
│   ├── gl_viewport.hpp             # NEW: QQuickFramebufferObject
│   ├── gl_viewport_renderer.hpp    # NEW: Renderer (render thread)
│   ├── camera_state.hpp            # NEW: CameraState
│   └── trackball_controller.hpp    # NEW: TrackballController
├── src/
│   ├── gl_viewport.cpp             # NEW
│   ├── gl_viewport_renderer.cpp    # NEW
│   ├── camera_state.cpp            # NEW
│   └── trackball_controller.cpp    # NEW
└── resource/qml/sections/
    └── ViewportPanel.qml           # MODIFY: embed GLViewport
```

### Modified: Root

```
CMakeLists.txt                      # MODIFY: add_subdirectory(src/libs/render)
```

---

## Task 0: Add glad via CPM & Create render lib CMake skeleton

**Files:**
- Create: `src/libs/render/CMakeLists.txt`
- Modify: `CMakeLists.txt` (root, line ~255)

- [ ] **Step 1: Add glad CPM dependency to root CMakeLists.txt**

In root `CMakeLists.txt`, after the existing CPM dependencies (around line 200-240 area where other CPMAddPackage calls live), add:

```cmake
CPMAddPackage(
    NAME glad
    GITHUB_REPOSITORY Dav1dde/glad
    VERSION 2.0.8
    OPTIONS "GLAD_API gl:core=3.3")
```

> Note: Search the root CMakeLists.txt for existing CPMAddPackage calls to find the right insertion point.

- [ ] **Step 2: Add `add_subdirectory(src/libs/render)` to root CMakeLists.txt**

In root `CMakeLists.txt`, insert after `add_subdirectory(src/libs/scene)` (line 255):

```cmake
add_subdirectory(src/libs/render)
```

Result:
```cmake
add_subdirectory(src/libs/core)
add_subdirectory(src/libs/io)
add_subdirectory(src/libs/geometry)
add_subdirectory(src/libs/scene)
add_subdirectory(src/libs/render)
add_subdirectory(src/libs/command)
add_subdirectory(src/libs/python)
add_subdirectory(src/app)
```

- [ ] **Step 3: Create `src/libs/render/CMakeLists.txt`**

```cmake
# ---------------------------------------------------------------------------
# OpenGeoLab Render Library — pure OpenGL, no Qt dependency
# ---------------------------------------------------------------------------

set(render_public_headers
    include/opengeolab/render/frame_state.hpp
    include/opengeolab/render/pick_mask.hpp
    include/opengeolab/render/pick_result.hpp
    include/opengeolab/render/render_pipeline.hpp
    include/opengeolab/render/batch_utils.hpp)

set(render_sources
    src/render_pipeline.cpp
    src/core/shader_program.cpp
    src/core/gpu_buffer_manager.cpp
    src/core/pick_fbo.cpp
    src/pass/opaque_pass.cpp
    src/pass/wireframe_pass.cpp
    src/pass/highlight_pass.cpp
    src/pass/selection_pass.cpp
    src/pick_resolver.cpp
    src/batch_utils.cpp)

opengeolab_add_module(
    opengeolab_render
    ALIAS_NAME
    Render
    SOURCES
    ${render_sources}
    PUBLIC_HEADERS
    ${render_public_headers}
    PUBLIC_LINKS
    OpenGeoLab::Core
    OpenGeoLab::Scene
    glad::glad
    glm::glm)

# Internal headers in src/ are accessible within the lib
target_include_directories(opengeolab_render
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

if (OPENGEOLAB_BUILD_TESTS)
    opengeolab_add_doctest_test(
        opengeolab_pick_resolver_test
        SOURCES test/pick_resolver_test.cpp
        LINKS OpenGeoLab::Render)

    opengeolab_add_doctest_test(
        opengeolab_batch_utils_test
        SOURCES test/batch_utils_test.cpp
        LINKS OpenGeoLab::Render)
endif ()
```

- [ ] **Step 4: Create minimal stub public headers so the lib compiles**

Create each public header with minimal content (just namespace and empty struct/class):

`include/opengeolab/render/frame_state.hpp`:
```cpp
/**
 * @file frame_state.hpp
 * @brief Per-frame rendering state passed to each render pass
 */

#pragma once

#include <opengeolab/scene/display_mode.hpp>
#include <opengeolab/scene/render_mesh_data.hpp>

#include <glm/glm.hpp>

#include <vector>

namespace OpenGeoLab::Render {

struct FrameState {
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projMatrix{1.0f};
    glm::vec3 cameraPos{0.0f};
    float devicePixelRatio{1.0f};

    int viewportWidth{0};
    int viewportHeight{0};

    bool xRayMode{false};
    Scene::DisplayModeMask displayMask{
        Scene::DisplayModeMask::Surface | Scene::DisplayModeMask::Wireframe};

    std::vector<Scene::DrawRange> selectedDrawRanges;
    std::vector<Scene::DrawRange> hoveredDrawRanges;
};

} // namespace OpenGeoLab::Render
```

`include/opengeolab/render/pick_mask.hpp`:
```cpp
/**
 * @file pick_mask.hpp
 * @brief PickMode and PickMask enumerations for GPU pick filtering
 */

#pragma once

#include <cstdint>

namespace OpenGeoLab::Render {

enum class PickMode : uint8_t {
    VEF,   /**< Vertex > Edge > Face priority */
    Wire,  /**< Edge → resolve to Wire */
    Solid, /**< Face → resolve to Solid */
    Part,  /**< Any → resolve to Part (shapeId) */
};

enum class PickMask : uint32_t {
    None   = 0,
    Vertex = 1 << 0,
    Edge   = 1 << 1,
    Wire   = 1 << 2,
    Face   = 1 << 3,
    Solid  = 1 << 4,
    Part   = 1 << 5,
    All    = 0xFFFFFFFF,
};

constexpr PickMask operator|(PickMask a, PickMask b) {
    return static_cast<PickMask>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr PickMask operator&(PickMask a, PickMask b) {
    return static_cast<PickMask>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

} // namespace OpenGeoLab::Render
```

`include/opengeolab/render/pick_result.hpp`:
```cpp
/**
 * @file pick_result.hpp
 * @brief PickResult — resolved pick target
 */

#pragma once

#include <opengeolab/core/entity_tag.hpp>

#include <cstdint>

namespace OpenGeoLab::Render {

struct PickResult {
    uint32_t shapeId{0};
    Core::EntityType entityType{};
    uint32_t localId{0};
    bool valid{false};
};

} // namespace OpenGeoLab::Render
```

`include/opengeolab/render/render_pipeline.hpp`:
```cpp
/**
 * @file render_pipeline.hpp
 * @brief RenderPipeline — top-level rendering entry point
 */

#pragma once

#include <opengeolab/render/frame_state.hpp>
#include <opengeolab/render/pick_mask.hpp>
#include <opengeolab/render/pick_result.hpp>
#include <opengeolab/render/render_export.hpp>

#include <vector>

namespace OpenGeoLab::Scene {
class SceneGraph;
class TopologyIndex;
} // namespace OpenGeoLab::Scene

namespace OpenGeoLab::Render {

class OPENGEOLAB_RENDER_EXPORT RenderPipeline final {
public:
    RenderPipeline();
    ~RenderPipeline();

    void initialize();
    void resize(int width, int height);
    void synchronize(const Scene::SceneGraph& scene);
    void render(const FrameState& state);

    PickResult pickAt(int x, int y, PickMask mask) const;
    std::vector<PickResult> pickRegion(int cx, int cy, int radius, PickMask mask) const;

    void cleanup();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace OpenGeoLab::Render
```

`include/opengeolab/render/batch_utils.hpp`:
```cpp
/**
 * @file batch_utils.hpp
 * @brief Batch building utilities for glMultiDraw* calls
 */

#pragma once

#include <opengeolab/scene/render_mesh_data.hpp>

#include <glad/gl.h>

#include <vector>

namespace OpenGeoLab::Render::BatchUtils {

struct IndexedBatch {
    std::vector<GLsizei> counts;
    std::vector<const void*> offsets;
    [[nodiscard]] GLsizei drawCount() const {
        return static_cast<GLsizei>(counts.size());
    }
};

struct ArrayBatch {
    std::vector<GLint> firsts;
    std::vector<GLsizei> counts;
    [[nodiscard]] GLsizei drawCount() const {
        return static_cast<GLsizei>(counts.size());
    }
};

template <typename Predicate>
IndexedBatch buildIndexedBatch(const std::vector<Scene::DrawRange>& ranges,
                               Predicate&& predicate);

template <typename Predicate>
ArrayBatch buildArrayBatch(const std::vector<Scene::DrawRange>& ranges,
                           Predicate&& predicate);

void multiDrawElements(GLenum mode, const IndexedBatch& batch);
void multiDrawArrays(GLenum mode, const ArrayBatch& batch);

} // namespace OpenGeoLab::Render::BatchUtils
```

- [ ] **Step 5: Create minimal stub .cpp files so the lib compiles**

Create all the `.cpp` source files with empty implementations. Each file should include its header and have stub function bodies. Example for `src/render_pipeline.cpp`:

```cpp
#include <opengeolab/render/render_pipeline.hpp>

namespace OpenGeoLab::Render {

struct RenderPipeline::Impl {};

RenderPipeline::RenderPipeline() : m_impl(std::make_unique<Impl>()) {}
RenderPipeline::~RenderPipeline() = default;

void RenderPipeline::initialize() {}
void RenderPipeline::resize(int /*width*/, int /*height*/) {}
void RenderPipeline::synchronize(const Scene::SceneGraph& /*scene*/) {}
void RenderPipeline::render(const FrameState& /*state*/) {}

PickResult RenderPipeline::pickAt(int /*x*/, int /*y*/, PickMask /*mask*/) const {
    return {};
}
std::vector<PickResult> RenderPipeline::pickRegion(int /*cx*/, int /*cy*/,
                                                    int /*radius*/, PickMask /*mask*/) const {
    return {};
}
void RenderPipeline::cleanup() {}

} // namespace OpenGeoLab::Render
```

Create similar stubs for:
- `src/core/shader_program.hpp` + `src/core/shader_program.cpp`
- `src/core/gpu_buffer_manager.hpp` + `src/core/gpu_buffer_manager.cpp`
- `src/core/pick_fbo.hpp` + `src/core/pick_fbo.cpp`
- `src/pass/render_pass_base.hpp`
- `src/pass/opaque_pass.hpp` + `src/pass/opaque_pass.cpp`
- `src/pass/wireframe_pass.hpp` + `src/pass/wireframe_pass.cpp`
- `src/pass/highlight_pass.hpp` + `src/pass/highlight_pass.cpp`
- `src/pass/selection_pass.hpp` + `src/pass/selection_pass.cpp`
- `src/pick_resolver.hpp` + `src/pick_resolver.cpp`
- `src/batch_utils.cpp`

And empty test files:
- `test/pick_resolver_test.cpp`
- `test/batch_utils_test.cpp`

- [ ] **Step 6: Configure and build to verify the skeleton compiles**

Run:
```
cmake --preset relwithdebinfo
cmake --build build --config RelWithDebInfo --parallel 8
```
Expected: Build succeeds with all stub files compiled and linked.

- [ ] **Step 7: Run tests to verify no regressions**

Run:
```
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```
Expected: All existing tests PASS. New tests also PASS (empty test files with no assertions).

- [ ] **Step 8: Commit**

```
git add src/libs/render/ CMakeLists.txt
git commit -m "build(render): add opengeolab_render library skeleton with glad

Create the render module directory structure with:
- CMakeLists.txt using opengeolab_add_module()
- Stub public headers (FrameState, PickMask, PickResult, RenderPipeline, BatchUtils)
- Stub source files for all internal components
- Two test targets (pick_resolver, batch_utils)
- glad 2.x via CPM for GL function loading

All stubs compile but contain no real logic yet.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 1: ShaderProgram — GLSL compile/link utility

**Files:**
- Create: `src/libs/render/src/core/shader_program.hpp`
- Create: `src/libs/render/src/core/shader_program.cpp`

- [ ] **Step 1: Implement ShaderProgram header**

`src/libs/render/src/core/shader_program.hpp`:
```cpp
/**
 * @file shader_program.hpp
 * @brief GLSL shader program compile, link, and uniform upload
 */

#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <string>
#include <string_view>

namespace OpenGeoLab::Render {

/**
 * @brief Compiles a vertex + fragment shader pair and exposes uniform setters.
 *
 * Lifetime: call create() once, use() per frame, destroy() on cleanup.
 * All GL calls require a current context.
 */
class ShaderProgram final {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    /**
     * @brief Compile vertex + fragment source and link into a program.
     * @return true on success, false on compile/link error (logged via Core::getLogger).
     */
    bool create(std::string_view vertexSrc, std::string_view fragmentSrc);

    /** @brief Activate this program for subsequent draw calls. */
    void use() const;

    /** @brief Delete the GL program object. */
    void destroy();

    /** @brief Raw GL program id (0 if not created). */
    [[nodiscard]] GLuint id() const { return m_program; }

    // ── Uniform setters ──

    void setMat4(std::string_view name, const glm::mat4& value) const;
    void setVec3(std::string_view name, const glm::vec3& value) const;
    void setVec4(std::string_view name, const glm::vec4& value) const;
    void setFloat(std::string_view name, float value) const;
    void setInt(std::string_view name, int value) const;

private:
    /** @brief Compile a single shader stage. Returns 0 on error. */
    static GLuint compileShader(GLenum type, std::string_view source);

    GLuint m_program{0};
};

} // namespace OpenGeoLab::Render
```

- [ ] **Step 2: Implement ShaderProgram source**

`src/libs/render/src/core/shader_program.cpp`:
```cpp
#include "core/shader_program.hpp"

#include <opengeolab/core/logger.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <utility>

namespace OpenGeoLab::Render {

ShaderProgram::~ShaderProgram() { destroy(); }

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : m_program(std::exchange(other.m_program, 0)) {}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
    if (this != &other) {
        destroy();
        m_program = std::exchange(other.m_program, 0);
    }
    return *this;
}

GLuint ShaderProgram::compileShader(GLenum type, std::string_view source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.data();
    auto length = static_cast<GLint>(source.size());
    glShaderSource(shader, 1, &src, &length);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(static_cast<std::string::size_type>(logLen), '\0');
        glGetShaderInfoLog(shader, logLen, nullptr, log.data());
        Core::getLogger()->error("Shader compile error: {}", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool ShaderProgram::create(std::string_view vertexSrc, std::string_view fragmentSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSrc);
    if (vs == 0) return false;

    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    if (fs == 0) {
        glDeleteShader(vs);
        return false;
    }

    m_program = glCreateProgram();
    glAttachShader(m_program, vs);
    glAttachShader(m_program, fs);
    glLinkProgram(m_program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint success = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLint logLen = 0;
        glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(static_cast<std::string::size_type>(logLen), '\0');
        glGetProgramInfoLog(m_program, logLen, nullptr, log.data());
        Core::getLogger()->error("Shader link error: {}", log);
        glDeleteProgram(m_program);
        m_program = 0;
        return false;
    }
    return true;
}

void ShaderProgram::use() const {
    glUseProgram(m_program);
}

void ShaderProgram::destroy() {
    if (m_program != 0) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

void ShaderProgram::setMat4(std::string_view name, const glm::mat4& value) const {
    GLint loc = glGetUniformLocation(m_program, std::string(name).c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderProgram::setVec3(std::string_view name, const glm::vec3& value) const {
    GLint loc = glGetUniformLocation(m_program, std::string(name).c_str());
    glUniform3fv(loc, 1, glm::value_ptr(value));
}

void ShaderProgram::setVec4(std::string_view name, const glm::vec4& value) const {
    GLint loc = glGetUniformLocation(m_program, std::string(name).c_str());
    glUniform4fv(loc, 1, glm::value_ptr(value));
}

void ShaderProgram::setFloat(std::string_view name, float value) const {
    GLint loc = glGetUniformLocation(m_program, std::string(name).c_str());
    glUniform1f(loc, value);
}

void ShaderProgram::setInt(std::string_view name, int value) const {
    GLint loc = glGetUniformLocation(m_program, std::string(name).c_str());
    glUniform1i(loc, value);
}

} // namespace OpenGeoLab::Render
```

- [ ] **Step 3: Build to verify**

```
cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 8
```
Expected: PASS

- [ ] **Step 4: Commit**

```
git add src/libs/render/src/core/shader_program.hpp src/libs/render/src/core/shader_program.cpp
git commit -m "feat(render): implement ShaderProgram compile/link utility

Compile vertex + fragment GLSL, link program, provide uniform setters
for mat4, vec3, vec4, float, int. Error logging via Core::getLogger().

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 2: BatchUtils — glMultiDraw batch building (TDD)

**Files:**
- Modify: `src/libs/render/include/opengeolab/render/batch_utils.hpp` (from Task 0 stub)
- Modify: `src/libs/render/src/batch_utils.cpp`
- Modify: `src/libs/render/test/batch_utils_test.cpp`

- [ ] **Step 1: Write the failing tests**

`test/batch_utils_test.cpp`:
```cpp
#include <opengeolab/render/batch_utils.hpp>

#include <doctest/doctest.h>

using OpenGeoLab::Render::BatchUtils::ArrayBatch;
using OpenGeoLab::Render::BatchUtils::IndexedBatch;
using OpenGeoLab::Render::BatchUtils::buildArrayBatch;
using OpenGeoLab::Render::BatchUtils::buildIndexedBatch;
using OpenGeoLab::Scene::DrawRange;
using OpenGeoLab::Scene::PrimitiveTopology;

TEST_CASE("buildIndexedBatch — empty input") {
    std::vector<DrawRange> ranges;
    auto batch = buildIndexedBatch(ranges, [](const DrawRange&) { return true; });
    CHECK(batch.drawCount() == 0);
    CHECK(batch.counts.empty());
    CHECK(batch.offsets.empty());
}

TEST_CASE("buildIndexedBatch — all accepted") {
    std::vector<DrawRange> ranges(2);
    ranges[0].indexOffset = 0;
    ranges[0].indexCount = 36;
    ranges[0].topology = PrimitiveTopology::Triangles;
    ranges[1].indexOffset = 36;
    ranges[1].indexCount = 24;
    ranges[1].topology = PrimitiveTopology::Triangles;

    auto batch = buildIndexedBatch(ranges, [](const DrawRange&) { return true; });
    CHECK(batch.drawCount() == 2);
    CHECK(batch.counts[0] == 36);
    CHECK(batch.counts[1] == 24);
    // offsets are byte offsets into IBO (uint32_t = 4 bytes)
    CHECK(batch.offsets[0] == reinterpret_cast<const void*>(0 * sizeof(uint32_t)));
    CHECK(batch.offsets[1] == reinterpret_cast<const void*>(36 * sizeof(uint32_t)));
}

TEST_CASE("buildIndexedBatch — predicate filters some") {
    std::vector<DrawRange> ranges(3);
    ranges[0].indexOffset = 0;
    ranges[0].indexCount = 10;
    ranges[0].shapeId = 1;
    ranges[1].indexOffset = 10;
    ranges[1].indexCount = 20;
    ranges[1].shapeId = 2;
    ranges[2].indexOffset = 30;
    ranges[2].indexCount = 15;
    ranges[2].shapeId = 1;

    auto batch = buildIndexedBatch(ranges, [](const DrawRange& r) { return r.shapeId == 1; });
    CHECK(batch.drawCount() == 2);
    CHECK(batch.counts[0] == 10);
    CHECK(batch.counts[1] == 15);
}

TEST_CASE("buildArrayBatch — all accepted") {
    std::vector<DrawRange> ranges(2);
    ranges[0].vertexOffset = 0;
    ranges[0].vertexCount = 100;
    ranges[1].vertexOffset = 100;
    ranges[1].vertexCount = 50;

    auto batch = buildArrayBatch(ranges, [](const DrawRange&) { return true; });
    CHECK(batch.drawCount() == 2);
    CHECK(batch.firsts[0] == 0);
    CHECK(batch.counts[0] == 100);
    CHECK(batch.firsts[1] == 100);
    CHECK(batch.counts[1] == 50);
}

TEST_CASE("buildArrayBatch — empty after filter") {
    std::vector<DrawRange> ranges(1);
    ranges[0].vertexOffset = 0;
    ranges[0].vertexCount = 10;

    auto batch = buildArrayBatch(ranges, [](const DrawRange&) { return false; });
    CHECK(batch.drawCount() == 0);
}
```

- [ ] **Step 2: Run tests to verify they fail**

```
cmake --build build --config RelWithDebInfo --target opengeolab_batch_utils_test --parallel 8
ctest --test-dir build -C RelWithDebInfo -R batch_utils --output-on-failure
```
Expected: FAIL (stub implementations)

- [ ] **Step 3: Implement BatchUtils**

`src/batch_utils.cpp`:
```cpp
#include <opengeolab/render/batch_utils.hpp>

namespace OpenGeoLab::Render::BatchUtils {

template <typename Predicate>
IndexedBatch buildIndexedBatch(const std::vector<Scene::DrawRange>& ranges,
                               Predicate&& predicate) {
    IndexedBatch batch;
    for (const auto& r : ranges) {
        if (predicate(r)) {
            batch.counts.push_back(static_cast<GLsizei>(r.indexCount));
            batch.offsets.push_back(
                reinterpret_cast<const void*>(
                    static_cast<uintptr_t>(r.indexOffset) * sizeof(uint32_t)));
        }
    }
    return batch;
}

template <typename Predicate>
ArrayBatch buildArrayBatch(const std::vector<Scene::DrawRange>& ranges,
                           Predicate&& predicate) {
    ArrayBatch batch;
    for (const auto& r : ranges) {
        if (predicate(r)) {
            batch.firsts.push_back(static_cast<GLint>(r.vertexOffset));
            batch.counts.push_back(static_cast<GLsizei>(r.vertexCount));
        }
    }
    return batch;
}

void multiDrawElements(GLenum mode, const IndexedBatch& batch) {
    if (batch.drawCount() == 0) return;
    glMultiDrawElements(mode,
                        batch.counts.data(),
                        GL_UNSIGNED_INT,
                        batch.offsets.data(),
                        batch.drawCount());
}

void multiDrawArrays(GLenum mode, const ArrayBatch& batch) {
    if (batch.drawCount() == 0) return;
    glMultiDrawArrays(mode,
                      batch.firsts.data(),
                      batch.counts.data(),
                      batch.drawCount());
}

// Explicit template instantiations for common predicates
template IndexedBatch buildIndexedBatch(
    const std::vector<Scene::DrawRange>&,
    std::function<bool(const Scene::DrawRange&)>&&);
template ArrayBatch buildArrayBatch(
    const std::vector<Scene::DrawRange>&,
    std::function<bool(const Scene::DrawRange&)>&&);

} // namespace OpenGeoLab::Render::BatchUtils
```

> **Note:** Since templates with arbitrary predicates need to be visible at call site, move the template bodies into the header file `batch_utils.hpp` and keep only the non-template `multiDrawElements`/`multiDrawArrays` in the .cpp. Update the header accordingly.

- [ ] **Step 4: Run tests to verify they pass**

```
cmake --build build --config RelWithDebInfo --target opengeolab_batch_utils_test --parallel 8
ctest --test-dir build -C RelWithDebInfo -R batch_utils --output-on-failure
```
Expected: ALL PASS

- [ ] **Step 5: Commit**

```
git add src/libs/render/include/opengeolab/render/batch_utils.hpp src/libs/render/src/batch_utils.cpp src/libs/render/test/batch_utils_test.cpp
git commit -m "feat(render): implement BatchUtils for glMultiDraw batching

Build IndexedBatch/ArrayBatch from DrawRange vectors with predicate
filtering. 5 test cases covering empty, full, and filtered scenarios.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 3: PickResolver — topology-aware pick resolution (TDD)

**Files:**
- Modify: `src/libs/render/src/pick_resolver.hpp`
- Modify: `src/libs/render/src/pick_resolver.cpp`
- Modify: `src/libs/render/test/pick_resolver_test.cpp`

- [ ] **Step 1: Write the failing tests**

`test/pick_resolver_test.cpp`:
```cpp
#include "pick_resolver.hpp"

#include <opengeolab/render/pick_mask.hpp>
#include <opengeolab/render/pick_result.hpp>
#include <opengeolab/scene/pick_id.hpp>
#include <opengeolab/scene/topology_index.hpp>

#include <doctest/doctest.h>

using OpenGeoLab::Core::EntityType;
using OpenGeoLab::Render::PickMode;
using OpenGeoLab::Render::PickResolver;
using OpenGeoLab::Render::PickResult;
using OpenGeoLab::Scene::PickId;
using OpenGeoLab::Scene::TopologyIndex;

TEST_CASE("PickResolver — empty input returns invalid") {
    TopologyIndex topo;
    PickResolver resolver(topo);
    auto result = resolver.resolve({}, PickMode::VEF);
    CHECK_FALSE(result.valid);
}

TEST_CASE("PickResolver — zero pickIds are skipped") {
    TopologyIndex topo;
    PickResolver resolver(topo);
    auto result = resolver.resolve({0, 0, 0}, PickMode::VEF);
    CHECK_FALSE(result.valid);
}

TEST_CASE("PickResolver — VEF mode: vertex wins over edge and face") {
    TopologyIndex topo;
    PickResolver resolver(topo);

    uint64_t face = PickId::encode(1, EntityType::GeoFace, 10);
    uint64_t edge = PickId::encode(1, EntityType::GeoEdge, 5);
    uint64_t vertex = PickId::encode(1, EntityType::GeoVertex, 2);

    // Input ordered face, edge, vertex — resolver should pick vertex
    auto result = resolver.resolve({face, edge, vertex}, PickMode::VEF);
    REQUIRE(result.valid);
    CHECK(result.entityType == EntityType::GeoVertex);
    CHECK(result.localId == 2);
    CHECK(result.shapeId == 1);
}

TEST_CASE("PickResolver — VEF mode: edge wins over face") {
    TopologyIndex topo;
    PickResolver resolver(topo);

    uint64_t face = PickId::encode(1, EntityType::GeoFace, 10);
    uint64_t edge = PickId::encode(1, EntityType::GeoEdge, 5);

    auto result = resolver.resolve({face, edge}, PickMode::VEF);
    REQUIRE(result.valid);
    CHECK(result.entityType == EntityType::GeoEdge);
    CHECK(result.localId == 5);
}

TEST_CASE("PickResolver — Part mode: returns shapeId") {
    TopologyIndex topo;
    PickResolver resolver(topo);

    uint64_t edge = PickId::encode(42, EntityType::GeoEdge, 7);
    auto result = resolver.resolve({edge}, PickMode::Part);
    REQUIRE(result.valid);
    CHECK(result.shapeId == 42);
}

TEST_CASE("PickResolver — resolveAll returns unique results") {
    TopologyIndex topo;
    PickResolver resolver(topo);

    uint64_t e1 = PickId::encode(1, EntityType::GeoEdge, 1);
    uint64_t e2 = PickId::encode(1, EntityType::GeoEdge, 2);
    uint64_t e1_dup = PickId::encode(1, EntityType::GeoEdge, 1);

    auto results = resolver.resolveAll({e1, e2, e1_dup}, PickMode::VEF);
    CHECK(results.size() == 2); // e1 de-duplicated
}
```

- [ ] **Step 2: Run tests to verify they fail**

```
cmake --build build --config RelWithDebInfo --target opengeolab_pick_resolver_test --parallel 8
ctest --test-dir build -C RelWithDebInfo -R pick_resolver --output-on-failure
```
Expected: FAIL

- [ ] **Step 3: Implement PickResolver header**

`src/pick_resolver.hpp`:
```cpp
/**
 * @file pick_resolver.hpp
 * @brief Resolves raw GPU pick IDs into typed pick results
 */

#pragma once

#include <opengeolab/render/pick_mask.hpp>
#include <opengeolab/render/pick_result.hpp>
#include <opengeolab/scene/render_mesh_data.hpp>
#include <opengeolab/scene/topology_index.hpp>

#include <cstdint>
#include <vector>

namespace OpenGeoLab::Render {

class PickResolver final {
public:
    explicit PickResolver(const Scene::TopologyIndex& topoIndex);

    /**
     * @brief Resolve a list of raw pickIds to a single best result.
     *
     * VEF mode: priority Vertex > Edge > Face.
     * Wire mode: resolve Edge → Wire via TopologyIndex.
     * Solid mode: resolve Face → Solid via TopologyIndex.
     * Part mode: return shapeId from first valid hit.
     *
     * Input is assumed sorted by distance from pick center (center-first).
     */
    [[nodiscard]] PickResult resolve(const std::vector<uint64_t>& rawPickIds,
                                     PickMode mode) const;

    /** @brief Resolve all unique entities for box-select. */
    [[nodiscard]] std::vector<PickResult> resolveAll(
        const std::vector<uint64_t>& rawPickIds,
        PickMode mode) const;

private:
    [[nodiscard]] PickResult resolveOne(uint64_t pickId, PickMode mode) const;
    [[nodiscard]] static int typePriority(Core::EntityType type);

    const Scene::TopologyIndex& m_topoIndex;
};

} // namespace OpenGeoLab::Render
```

- [ ] **Step 4: Implement PickResolver source**

`src/pick_resolver.cpp`:
```cpp
#include "pick_resolver.hpp"

#include <opengeolab/scene/pick_id.hpp>

#include <algorithm>
#include <unordered_set>

namespace OpenGeoLab::Render {

PickResolver::PickResolver(const Scene::TopologyIndex& topoIndex)
    : m_topoIndex(topoIndex) {}

int PickResolver::typePriority(Core::EntityType type) {
    switch (type) {
        case Core::EntityType::GeoVertex: return 3;
        case Core::EntityType::GeoEdge:   return 2;
        case Core::EntityType::GeoWire:   return 2;
        case Core::EntityType::GeoFace:   return 1;
        case Core::EntityType::GeoSolid:  return 1;
        default: return 0;
    }
}

PickResult PickResolver::resolveOne(uint64_t pickId, PickMode mode) const {
    if (!Scene::PickId::isValid(pickId)) return {};

    uint32_t shapeId = Scene::PickId::decodeShapeId(pickId);
    Core::EntityType type = Scene::PickId::decodeType(pickId);
    uint32_t localId = Scene::PickId::decodeLocalId(pickId);

    PickResult result{shapeId, type, localId, true};

    switch (mode) {
        case PickMode::Part:
            result.entityType = Core::EntityType::GeoSolid;
            result.localId = 0;
            break;
        case PickMode::Wire:
            if (type == Core::EntityType::GeoEdge) {
                if (auto wire = m_topoIndex.edgeToWire(shapeId, localId)) {
                    result.entityType = Core::EntityType::GeoWire;
                    result.localId = *wire;
                }
            }
            break;
        case PickMode::Solid:
            if (type == Core::EntityType::GeoFace) {
                if (auto solid = m_topoIndex.faceToSolid(shapeId, localId)) {
                    result.entityType = Core::EntityType::GeoSolid;
                    result.localId = *solid;
                }
            }
            break;
        case PickMode::VEF:
            break; // Return as-is; caller picks highest priority
    }
    return result;
}

PickResult PickResolver::resolve(const std::vector<uint64_t>& rawPickIds,
                                  PickMode mode) const {
    PickResult best;
    int bestPriority = -1;

    for (uint64_t id : rawPickIds) {
        auto candidate = resolveOne(id, mode);
        if (!candidate.valid) continue;

        if (mode == PickMode::Part || mode == PickMode::Wire || mode == PickMode::Solid) {
            return candidate; // First valid hit wins (center-first ordering)
        }

        int prio = typePriority(candidate.entityType);
        if (prio > bestPriority) {
            best = candidate;
            bestPriority = prio;
        }
    }
    return best;
}

std::vector<PickResult> PickResolver::resolveAll(
    const std::vector<uint64_t>& rawPickIds, PickMode mode) const {

    std::vector<PickResult> results;
    std::unordered_set<uint64_t> seen;

    for (uint64_t id : rawPickIds) {
        if (!Scene::PickId::isValid(id) || seen.count(id) > 0) continue;
        seen.insert(id);

        auto result = resolveOne(id, mode);
        if (result.valid) {
            results.push_back(result);
        }
    }
    return results;
}

} // namespace OpenGeoLab::Render
```

- [ ] **Step 5: Run tests to verify they pass**

```
cmake --build build --config RelWithDebInfo --target opengeolab_pick_resolver_test --parallel 8
ctest --test-dir build -C RelWithDebInfo -R pick_resolver --output-on-failure
```
Expected: ALL PASS

- [ ] **Step 6: Commit**

```
git add src/libs/render/src/pick_resolver.hpp src/libs/render/src/pick_resolver.cpp src/libs/render/test/pick_resolver_test.cpp
git commit -m "feat(render): implement PickResolver with VEF/Wire/Solid/Part modes

Resolves raw 64-bit GPU pickIds via PickId decode + TopologyIndex
lookup. VEF uses Vertex>Edge>Face priority, Part returns shapeId.
resolveAll deduplicates for box-select.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 4: GpuBufferManager — VBO/IBO upload and VAO management

**Files:**
- Modify: `src/libs/render/src/core/gpu_buffer_manager.hpp`
- Modify: `src/libs/render/src/core/gpu_buffer_manager.cpp`

- [ ] **Step 1: Implement GpuBufferManager header**

`src/core/gpu_buffer_manager.hpp`:
```cpp
/**
 * @file gpu_buffer_manager.hpp
 * @brief Manages VAO/VBO/IBO for scene geometry on the GPU
 */

#pragma once

#include <opengeolab/scene/render_mesh_data.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <glad/gl.h>

#include <cstdint>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief Uploads scene RenderMeshData to GPU buffers.
 *
 * Maintains two VAO configurations:
 *   Main VAO: position + normal + color (for visible passes)
 *   Pick VAO: position + pickId (for selection pass)
 *
 * Checks SceneGraph version on each synchronize() and re-uploads
 * only when data has changed.
 */
class GpuBufferManager final {
public:
    void initialize();
    void cleanup();

    /**
     * @brief Traverse visible nodes and upload changed data to GPU.
     *
     * Caller must hold SceneGraph read lock.
     */
    void synchronize(const Scene::SceneGraph& scene);

    /** @brief Bind the main rendering VAO (pos+normal+color, IBO). */
    void bindMainVao() const;

    /** @brief Bind the pick VAO (pos+pickId, IBO). */
    void bindPickVao() const;

    /** @brief Unbind VAO. */
    void unbind() const;

    [[nodiscard]] const std::vector<Scene::DrawRange>& triangleRanges() const;
    [[nodiscard]] const std::vector<Scene::DrawRange>& lineRanges() const;
    [[nodiscard]] const std::vector<Scene::DrawRange>& pointRanges() const;

    [[nodiscard]] bool hasData() const;

private:
    void rebuildBuffers(const Scene::SceneGraph& scene);
    void setupMainVao();
    void setupPickVao();

    GLuint m_mainVao{0};
    GLuint m_pickVao{0};
    GLuint m_mainVbo{0};
    GLuint m_pickVbo{0};
    GLuint m_ibo{0};

    uint64_t m_uploadedVersion{0};

    std::vector<Scene::DrawRange> m_triangleRanges;
    std::vector<Scene::DrawRange> m_lineRanges;
    std::vector<Scene::DrawRange> m_pointRanges;

    bool m_hasData{false};
};

} // namespace OpenGeoLab::Render
```

- [ ] **Step 2: Implement GpuBufferManager source**

`src/core/gpu_buffer_manager.cpp` — full implementation that:
1. On `initialize()`: creates 2 VAOs, 3 buffers (mainVbo, pickVbo, ibo)
2. On `synchronize()`: checks `scene.version()` vs `m_uploadedVersion`, calls `rebuildBuffers()` if dirty
3. `rebuildBuffers()`: traverses visible nodes, collects all RenderVertex/PickIdEntry/indices, adjusts DrawRange offsets for global VBO, uploads via `glBufferData`
4. `setupMainVao()`: binds position (location 0, 3 floats, stride 40), normal (location 1, 3 floats, offset 12), color (location 2, 4 floats, offset 24)
5. `setupPickVao()`: binds position from mainVbo (location 0), pickId from pickVbo as uvec2 (location 1, 2 × uint32, stride 8)
6. On `cleanup()`: deletes all GL objects

Key vertex attribute layout for main VAO:
```cpp
// RenderVertex: float[3] pos, float[3] normal, float[4] color = 40 bytes
constexpr GLsizei kMainStride = sizeof(Scene::RenderVertex); // 40

// location 0: position
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, kMainStride,
                      reinterpret_cast<const void*>(0));
// location 1: normal
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, kMainStride,
                      reinterpret_cast<const void*>(3 * sizeof(float)));
// location 2: color
glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, kMainStride,
                      reinterpret_cast<const void*>(6 * sizeof(float)));
```

Key vertex attribute layout for pick VAO:
```cpp
// location 0: position (from mainVbo)
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, kMainStride,
                      reinterpret_cast<const void*>(0));
// location 1: pickId as uvec2 (from pickVbo)
constexpr GLsizei kPickStride = sizeof(Scene::PickIdEntry); // 8
glVertexAttribIPointer(1, 2, GL_UNSIGNED_INT, kPickStride,
                       reinterpret_cast<const void*>(0));
```

- [ ] **Step 3: Build to verify**

```
cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 8
```
Expected: PASS

- [ ] **Step 4: Commit**

```
git add src/libs/render/src/core/gpu_buffer_manager.hpp src/libs/render/src/core/gpu_buffer_manager.cpp
git commit -m "feat(render): implement GpuBufferManager for VBO/IBO upload

Two-VAO design: Main (pos+normal+color, 40B/vtx) and Pick (pos+pickId,
8B/vtx). Version-tracked dirty sync from SceneGraph. Concatenates all
visible node mesh data with offset-adjusted DrawRanges.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 5: PickFbo — off-screen pick framebuffer

**Files:**
- Modify: `src/libs/render/src/core/pick_fbo.hpp`
- Modify: `src/libs/render/src/core/pick_fbo.cpp`

- [ ] **Step 1: Implement PickFbo header**

`src/core/pick_fbo.hpp`:
```cpp
/**
 * @file pick_fbo.hpp
 * @brief Off-screen FBO for GPU color picking (RG32UI + depth)
 */

#pragma once

#include <glad/gl.h>

#include <cstdint>
#include <vector>

namespace OpenGeoLab::Render {

class PickFbo final {
public:
    bool initialize(int width, int height);
    void resize(int width, int height);
    void cleanup();

    void bind() const;
    void unbind() const;

    /** @brief Read a single pixel's pickId. */
    [[nodiscard]] uint64_t readPickId(int x, int y) const;

    /**
     * @brief Read a region centered at (cx,cy) with given radius.
     *
     * Returns non-zero pickIds sorted by distance from center (center-first).
     * Default radius = 6 → 13×13 = 169 pixel region.
     */
    [[nodiscard]] std::vector<uint64_t> readPickRegion(int cx, int cy,
                                                        int radius = 6) const;

private:
    GLuint m_fbo{0};
    GLuint m_colorTex{0};
    GLuint m_depthRbo{0};
    int m_width{0};
    int m_height{0};
};

} // namespace OpenGeoLab::Render
```

- [ ] **Step 2: Implement PickFbo source**

`src/core/pick_fbo.cpp` — full implementation:
1. `initialize()`: create FBO, GL_RG32UI texture, GL_DEPTH_COMPONENT24 renderbuffer
2. `resize()`: recreate texture and renderbuffer at new size
3. `readPickId()`: `glReadPixels` 1×1 at (x, height-1-y) for GL coordinate flip, reconstruct uint64_t from RG32UI
4. `readPickRegion()`: clamp region to viewport bounds, single `glReadPixels` for (2r+1)×(2r+1), sort by Euclidean distance² from center, return non-zero pickIds

Key implementation for center-first sorting:
```cpp
std::vector<uint64_t> PickFbo::readPickRegion(int cx, int cy, int radius) const {
    int side = 2 * radius + 1;
    int x0 = std::max(cx - radius, 0);
    int y0 = std::max((m_height - 1 - cy) - radius, 0);
    int x1 = std::min(cx + radius, m_width - 1);
    int y1 = std::min((m_height - 1 - cy) + radius, m_height - 1);
    int w = x1 - x0 + 1;
    int h = y1 - y0 + 1;
    if (w <= 0 || h <= 0) return {};

    std::vector<uint32_t> data(static_cast<size_t>(w * h * 2));
    glReadPixels(x0, y0, w, h, GL_RG_INTEGER, GL_UNSIGNED_INT, data.data());

    int pixelCount = w * h;
    int centerX = cx - x0;
    int centerY = (m_height - 1 - cy) - y0;

    // Build (distance², index) pairs
    std::vector<std::pair<int, int>> order;
    order.reserve(static_cast<size_t>(pixelCount));
    for (int i = 0; i < pixelCount; ++i) {
        int px = i % w;
        int py = i / w;
        int dx = px - centerX;
        int dy = py - centerY;
        order.emplace_back(dx * dx + dy * dy, i);
    }
    std::sort(order.begin(), order.end());

    std::vector<uint64_t> result;
    for (auto [dist2, idx] : order) {
        uint32_t lo = data[static_cast<size_t>(idx) * 2];
        uint32_t hi = data[static_cast<size_t>(idx) * 2 + 1];
        uint64_t pickId = (static_cast<uint64_t>(hi) << 32u) | lo;
        if (pickId != 0) {
            result.push_back(pickId);
        }
    }
    return result;
}
```

- [ ] **Step 3: Build to verify**

```
cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 8
```
Expected: PASS

- [ ] **Step 4: Commit**

```
git add src/libs/render/src/core/pick_fbo.hpp src/libs/render/src/core/pick_fbo.cpp
git commit -m "feat(render): implement PickFbo with center-first readback

RG32UI color texture + DEPTH24 renderbuffer. readPickRegion() sorts
by Euclidean distance from center for cursor-priority picking.
Default radius=6 (13x13 pixel region).

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 6: RenderPassBase + OpaquePass

**Files:**
- Modify: `src/libs/render/src/pass/render_pass_base.hpp`
- Modify: `src/libs/render/src/pass/opaque_pass.hpp`
- Modify: `src/libs/render/src/pass/opaque_pass.cpp`

- [ ] **Step 1: Implement RenderPassBase**

`src/pass/render_pass_base.hpp`:
```cpp
/**
 * @file render_pass_base.hpp
 * @brief Abstract base for render passes
 */

#pragma once

#include <opengeolab/render/frame_state.hpp>

namespace OpenGeoLab::Render {

class GpuBufferManager;

class RenderPassBase {
public:
    virtual ~RenderPassBase() = default;

    void initialize() {
        if (!m_initialized) {
            m_initialized = onInitialize();
        }
    }

    void cleanup() {
        if (m_initialized) {
            onCleanup();
            m_initialized = false;
        }
    }

    virtual void render(const FrameState& state,
                        const GpuBufferManager& buffers) = 0;

protected:
    virtual bool onInitialize() = 0;
    virtual void onCleanup() {}

    bool m_initialized{false};
};

} // namespace OpenGeoLab::Render
```

- [ ] **Step 2: Implement OpaquePass header**

`src/pass/opaque_pass.hpp`:
```cpp
/**
 * @file opaque_pass.hpp
 * @brief Draws filled triangles with 4-component lighting model
 */

#pragma once

#include "render_pass_base.hpp"
#include "core/shader_program.hpp"

namespace OpenGeoLab::Render {

class OpaquePass final : public RenderPassBase {
public:
    void render(const FrameState& state,
                const GpuBufferManager& buffers) override;

protected:
    bool onInitialize() override;
    void onCleanup() override;

private:
    ShaderProgram m_shader;
};

} // namespace OpenGeoLab::Render
```

- [ ] **Step 3: Implement OpaquePass source**

`src/pass/opaque_pass.cpp` — full implementation with:
1. GLSL vertex shader: transforms position/normal to view space, passes through color
2. GLSL fragment shader: 4-component lighting (ambient=0.35, headlamp=0.55, sky=0.15, ground=0.05), premultiplied alpha output
3. GL state: enable depth test, polygon offset (2.0f, 10.0f), draw only when displayMask includes Surface
4. Bind main VAO, build IndexedBatch from triangleRanges, call multiDrawElements(GL_TRIANGLES)

Vertex shader source (embedded as constexpr string_view):
```glsl
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_color;
uniform mat4 u_mvp;
uniform mat4 u_modelView;
uniform mat3 u_normalMatrix;
out vec3 v_normal;
out vec4 v_color;
out vec3 v_viewPos;
void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_normal = normalize(u_normalMatrix * a_normal);
    v_color = a_color;
    v_viewPos = (u_modelView * vec4(a_position, 1.0)).xyz;
}
```

Fragment shader source:
```glsl
#version 330 core
in vec3 v_normal;
in vec4 v_color;
in vec3 v_viewPos;
uniform float u_alpha;
out vec4 fragColor;
void main() {
    vec3 N = normalize(v_normal);
    vec3 V = normalize(-v_viewPos);
    float ambient     = 0.35;
    float headlamp    = abs(dot(N, V));
    float skyLight    = max(dot(N, vec3(0.0, 1.0, 0.0)), 0.0) * 0.15;
    float groundBounce = max(dot(N, vec3(0.0, -1.0, 0.0)), 0.0) * 0.05;
    float lighting    = ambient + headlamp * 0.55 + skyLight + groundBounce;
    vec3 lit = v_color.rgb * min(lighting, 1.0);
    float a = v_color.a * u_alpha;
    fragColor = vec4(lit * a, a);
}
```

- [ ] **Step 4: Build to verify**

```
cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 8
```
Expected: PASS

- [ ] **Step 5: Commit**

```
git add src/libs/render/src/pass/render_pass_base.hpp src/libs/render/src/pass/opaque_pass.hpp src/libs/render/src/pass/opaque_pass.cpp
git commit -m "feat(render): implement RenderPassBase and OpaquePass

4-component lighting model (ambient + headlamp + sky + ground bounce)
with premultiplied alpha. Polygon offset (2.0, 10.0) for z-fighting.
Uses glMultiDrawElements for batched triangle rendering.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 7: WireframePass — edges and points

**Files:**
- Modify: `src/libs/render/src/pass/wireframe_pass.hpp`
- Modify: `src/libs/render/src/pass/wireframe_pass.cpp`

- [ ] **Step 1: Implement WireframePass header**

`src/pass/wireframe_pass.hpp`:
```cpp
/**
 * @file wireframe_pass.hpp
 * @brief Draws edges (GL_LINES) and vertex points (GL_POINTS)
 */

#pragma once

#include "render_pass_base.hpp"
#include "core/shader_program.hpp"

namespace OpenGeoLab::Render {

class WireframePass final : public RenderPassBase {
public:
    void render(const FrameState& state,
                const GpuBufferManager& buffers) override;

protected:
    bool onInitialize() override;
    void onCleanup() override;

private:
    ShaderProgram m_lineShader;
    ShaderProgram m_pointShader;
};

} // namespace OpenGeoLab::Render
```

- [ ] **Step 2: Implement WireframePass source**

`src/pass/wireframe_pass.cpp`:
1. Line shader: same vertex shader as opaque but simpler fragment (no lighting, pass-through color, premultiplied alpha)
2. Point shader: same but with `gl_PointSize`
3. GL state: `glLineWidth(1.5f)`, `glDepthFunc(GL_LEQUAL)` for edges on top of faces
4. Point GL state: `glEnable(GL_PROGRAM_POINT_SIZE)`, `gl_PointSize = 6.0` in shader
5. Draw: bind main VAO, build IndexedBatch from lineRanges → `glMultiDrawElements(GL_LINES)`, then ArrayBatch from pointRanges → `glMultiDrawArrays(GL_POINTS)`
6. Restore: `glLineWidth(1.0f)`, `glDepthFunc(GL_LESS)`

Line fragment shader:
```glsl
#version 330 core
in vec4 v_color;
uniform float u_alpha;
out vec4 fragColor;
void main() {
    float a = v_color.a * u_alpha;
    fragColor = vec4(v_color.rgb * a, a);
}
```

Point vertex shader adds:
```glsl
uniform float u_pointSize;
// in main(): gl_PointSize = u_pointSize;
```

- [ ] **Step 3: Build to verify**

```
cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 8
```
Expected: PASS

- [ ] **Step 4: Commit**

```
git add src/libs/render/src/pass/wireframe_pass.hpp src/libs/render/src/pass/wireframe_pass.cpp
git commit -m "feat(render): implement WireframePass for edges and points

GL_LINES with lineWidth=1.5, GL_POINTS with pointSize=6.0.
GL_LEQUAL depth for edges on top of opaque surfaces.
Premultiplied alpha output for Qt Quick compositing.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 8: HighlightPass — selection/hover overlay

**Files:**
- Modify: `src/libs/render/src/pass/highlight_pass.hpp`
- Modify: `src/libs/render/src/pass/highlight_pass.cpp`

- [ ] **Step 1: Implement HighlightPass**

Header pattern matches OpaquePass. Two shaders: one for face highlighting (with lighting + highlight color mix), one for edge/point highlighting (flat highlight color).

`render()` implementation:
1. If `state.selectedDrawRanges` empty AND `state.hoveredDrawRanges` empty → skip
2. Bind main VAO
3. Draw selected triangles: polygon offset, set u_highlightColor = (0.2, 0.4, 0.9, 0.6), use face highlight shader
4. Draw selected edges: lineWidth=1.5, depthFunc=LEQUAL, u_highlightColor as uniform
5. Draw hovered triangles: u_highlightColor = (0.4, 0.7, 1.0, 0.4)
6. Draw hovered edges similarly

Face highlight fragment shader (from spec §4.4.4):
```glsl
#version 330 core
in vec3 v_normal;
in vec4 v_color;
in vec3 v_viewPos;
uniform float u_alpha;
uniform vec4 u_highlightColor;
out vec4 fragColor;
void main() {
    vec3 N = normalize(v_normal);
    vec3 V = normalize(-v_viewPos);
    float ambient     = 0.35;
    float headlamp    = abs(dot(N, V));
    float skyLight    = max(dot(N, vec3(0.0, 1.0, 0.0)), 0.0) * 0.15;
    float groundBounce = max(dot(N, vec3(0.0, -1.0, 0.0)), 0.0) * 0.05;
    float lighting    = ambient + headlamp * 0.55 + skyLight + groundBounce;
    vec3 litColor = v_color.rgb * min(lighting, 1.0);
    vec3 finalColor = mix(litColor, u_highlightColor.rgb, u_highlightColor.a);
    fragColor = vec4(finalColor * u_alpha, u_alpha);
}
```

- [ ] **Step 2: Build to verify**

```
cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 8
```
Expected: PASS

- [ ] **Step 3: Commit**

```
git add src/libs/render/src/pass/highlight_pass.hpp src/libs/render/src/pass/highlight_pass.cpp
git commit -m "feat(render): implement HighlightPass for selection/hover overlay

Redraws selected/hovered DrawRanges with highlight color mix.
Selected: (0.2, 0.4, 0.9, 0.6), Hovered: (0.4, 0.7, 1.0, 0.4).
Face highlights use full lighting model, edge highlights use flat color.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 9: SelectionPass — GPU pick rendering

**Files:**
- Modify: `src/libs/render/src/pass/selection_pass.hpp`
- Modify: `src/libs/render/src/pass/selection_pass.cpp`

- [ ] **Step 1: Implement SelectionPass**

1. Shader: vertex takes a_position (location 0) + a_pickId as uvec2 (location 1), fragment outputs `uvec2 fragPickId = v_pickId`
2. Binds pick FBO, clears to 0, renders ALL pickable geometry (tri + lines + points)
3. Uses pick VAO (position from mainVbo + pickId from pickVbo)
4. For edges: lineWidth = 4.0f (wider for easier picking)
5. For points: pointSize = 12.0f (larger for easier picking)

Selection vertex shader:
```glsl
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in uvec2 a_pickId;
uniform mat4 u_mvp;
flat out uvec2 v_pickId;
void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_pickId = a_pickId;
}
```

Selection fragment shader:
```glsl
#version 330 core
flat in uvec2 v_pickId;
layout(location = 0) out uvec2 fragPickId;
void main() {
    fragPickId = v_pickId;
}
```

- [ ] **Step 2: Build to verify**

```
cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 8
```
Expected: PASS

- [ ] **Step 3: Commit**

```
git add src/libs/render/src/pass/selection_pass.hpp src/libs/render/src/pass/selection_pass.cpp
git commit -m "feat(render): implement SelectionPass for GPU color picking

Renders all pickable geometry to RG32UI FBO with per-vertex pickId.
Pick edges use lineWidth=4.0, pick points use pointSize=12.0 for
comfortable hit target size.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 10: RenderPipeline — assemble the full pipeline

**Files:**
- Modify: `src/libs/render/src/render_pipeline.cpp`
- Modify: `src/libs/render/include/opengeolab/render/render_pipeline.hpp`

- [ ] **Step 1: Implement RenderPipeline::Impl with all components**

Replace the stub Impl with real members:
```cpp
struct RenderPipeline::Impl {
    GpuBufferManager bufferManager;
    OpaquePass opaquePass;
    WireframePass wireframePass;
    HighlightPass highlightPass;
    SelectionPass selectionPass;
    PickFbo pickFbo;
    PickResolver* pickResolver{nullptr}; // Set externally or constructed with TopologyIndex
    int viewportWidth{0};
    int viewportHeight{0};
    bool initialized{false};
};
```

- [ ] **Step 2: Implement all pipeline methods**

`initialize()`: init bufferManager, all 4 passes, pickFbo
`resize()`: resize pickFbo, store viewport dimensions
`synchronize()`: delegate to bufferManager.synchronize()
`render()`:
```cpp
void RenderPipeline::render(const FrameState& state) {
    if (!m_impl->initialized || !m_impl->bufferManager.hasData()) return;

    glViewport(0, 0, state.viewportWidth, state.viewportHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // transparent for Qt Quick
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    m_impl->opaquePass.render(state, m_impl->bufferManager);
    m_impl->wireframePass.render(state, m_impl->bufferManager);
    m_impl->highlightPass.render(state, m_impl->bufferManager);
    m_impl->selectionPass.render(state, m_impl->bufferManager);
}
```

`pickAt()`: read pickFbo.readPickRegion() → pickResolver.resolve()
`pickRegion()`: read larger region → pickResolver.resolveAll()
`cleanup()`: cleanup all sub-components

- [ ] **Step 3: Build to verify**

```
cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 8
```
Expected: PASS

- [ ] **Step 4: Run all tests**

```
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```
Expected: ALL PASS

- [ ] **Step 5: Commit**

```
git add src/libs/render/src/render_pipeline.cpp src/libs/render/include/opengeolab/render/render_pipeline.hpp
git commit -m "feat(render): assemble RenderPipeline with 4-pass architecture

Orchestrates: synchronize → OpaquePass → WireframePass →
HighlightPass → SelectionPass. Delegates picking to PickFbo +
PickResolver. Transparent clear for Qt Quick compositing.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 11: CameraState — Cartesian camera with orthographic projection

**Files:**
- Create: `src/app/include/opengeolab/app/camera_state.hpp`
- Create: `src/app/src/camera_state.cpp`

- [ ] **Step 1: Implement CameraState header**

`include/opengeolab/app/camera_state.hpp`:
```cpp
/**
 * @file camera_state.hpp
 * @brief Cartesian camera state with orthographic projection
 */

#pragma once

#include <opengeolab/scene/bounding_box3d.hpp>

#include <glm/glm.hpp>

namespace OpenGeoLab::App {

struct CameraState {
    glm::vec3 position{0.0f, 0.0f, 50.0f};
    glm::vec3 target{0.0f, 0.0f, 0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    float nearPlane{-500.0f};
    float farPlane{500.0f};

    [[nodiscard]] glm::mat4 viewMatrix() const;
    [[nodiscard]] glm::mat4 projMatrix(float aspect) const;
    [[nodiscard]] glm::vec3 eyePosition() const { return position; }
    [[nodiscard]] float distance() const;

    void updateClipping();
    void reset();
    void fitToBoundingBox(const Scene::BoundingBox3D& bounds);
};

} // namespace OpenGeoLab::App
```

- [ ] **Step 2: Implement CameraState source**

`src/camera_state.cpp`:
```cpp
#include <opengeolab/app/camera_state.hpp>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/component_wise.hpp>

#include <algorithm>
#include <cmath>

namespace OpenGeoLab::App {

glm::mat4 CameraState::viewMatrix() const {
    return glm::lookAt(position, target, up);
}

float CameraState::distance() const {
    return glm::length(position - target);
}

glm::mat4 CameraState::projMatrix(float aspect) const {
    float d = distance();
    float halfHeight = d * 0.5f;
    float halfWidth = halfHeight * aspect;
    return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight,
                      nearPlane, farPlane);
}

void CameraState::updateClipping() {
    float d = std::max(distance(), 1e-4f);
    float halfRange = d * 10.0f;
    nearPlane = -halfRange;
    farPlane = halfRange;
}

void CameraState::reset() {
    position = {0.0f, 0.0f, 50.0f};
    target = {0.0f, 0.0f, 0.0f};
    up = {0.0f, 1.0f, 0.0f};
    updateClipping();
}

void CameraState::fitToBoundingBox(const Scene::BoundingBox3D& bounds) {
    if (!bounds.isValid()) {
        reset();
        return;
    }
    target = bounds.center();
    float maxSize = glm::compMax(bounds.size());
    position = target + glm::vec3(0.0f, 0.0f, maxSize);
    updateClipping();
}

} // namespace OpenGeoLab::App
```

- [ ] **Step 3: Add to app CMakeLists.txt sources**

Add `src/camera_state.cpp` and header to the app target sources.

- [ ] **Step 4: Build to verify**

```
cmake --build build --config RelWithDebInfo --parallel 8
```
Expected: PASS

- [ ] **Step 5: Commit**

```
git add src/app/include/opengeolab/app/camera_state.hpp src/app/src/camera_state.cpp src/app/CMakeLists.txt
git commit -m "feat(app): implement CameraState with orthographic projection

Cartesian position/target/up model with symmetric clipping (±10×distance).
Orthographic projection: halfHeight = distance × 0.5.
fitToBoundingBox auto-positions camera to view entire scene.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 12: TrackballController — quaternion orbit/pan/zoom

**Files:**
- Create: `src/app/include/opengeolab/app/trackball_controller.hpp`
- Create: `src/app/src/trackball_controller.cpp`

- [ ] **Step 1: Implement TrackballController header**

Following the spec §5.4 design with quaternion orbit, exponential zoom, view-plane pan.

Key API:
```cpp
class TrackballController {
public:
    enum class Mode { None, Orbit, Pan, Zoom };

    void setViewportSize(const QSizeF& size);
    void syncFromCamera(const CameraState& state);
    bool isActive() const;
    Mode mode() const;

    void begin(const QPointF& pos, Mode mode, const CameraState& state);
    void update(const QPointF& pos, CameraState& state);
    void end();
    void wheelZoom(float steps, CameraState& state);

    void fitToScene(const Scene::BoundingBox3D& bounds, CameraState& state);

    enum class ViewPreset { Front, Back, Top, Bottom, Left, Right, Isometric };
    void setViewPreset(ViewPreset preset, CameraState& state);
};
```

- [ ] **Step 2: Implement TrackballController source**

Key constants (matching OGL):
```cpp
float m_orbitScale{2.2f};
float m_panScale{0.0015f};
float m_zoomSpeed{1.5f};
float m_zoomBase{0.90f};
float m_zoomPixelsPerStep{60.0f};
```

Orbit algorithm: project mouse to virtual sphere, compute rotation quaternion, accumulate, rebuild position/up from quaternion + distance.

Zoom: `distance *= pow(0.90, steps)`, clamp minimum 0.1.

Pan: delta pixels × distance × panScale → translate both position and target.

ViewPresets: set position relative to target for Front/Back/Top/Bottom/Left/Right/Isometric.

- [ ] **Step 3: Add to app CMakeLists.txt**

- [ ] **Step 4: Build to verify**

```
cmake --build build --config RelWithDebInfo --parallel 8
```
Expected: PASS

- [ ] **Step 5: Commit**

```
git add src/app/include/opengeolab/app/trackball_controller.hpp src/app/src/trackball_controller.cpp src/app/CMakeLists.txt
git commit -m "feat(app): implement TrackballController with quaternion orbit

Virtual-sphere orbit, exponential zoom (base=0.90), view-plane pan.
Constants aligned with OGL: orbitScale=2.2, panScale=0.0015,
zoomPixelsPerStep=60. Supports 7 view presets.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 13: GLViewport + GLViewportRenderer — Qt integration

**Files:**
- Create: `src/app/include/opengeolab/app/gl_viewport.hpp`
- Create: `src/app/include/opengeolab/app/gl_viewport_renderer.hpp`
- Create: `src/app/src/gl_viewport.cpp`
- Create: `src/app/src/gl_viewport_renderer.cpp`
- Modify: `src/app/CMakeLists.txt`

- [ ] **Step 1: Implement GLViewport header**

QQuickFramebufferObject subclass with:
- Q_PROPERTY: pickingEnabled, pickMode
- Q_INVOKABLE: fitToScene(), setViewPreset(int), toggleXRay()
- Signals: entityPicked, entityHovered, pickCleared
- Mouse/wheel/hover event overrides → TrackballController
- Internal: CameraState, TrackballController, pending pick position

- [ ] **Step 2: Implement GLViewportRenderer header**

QQuickFramebufferObject::Renderer subclass with:
- `createFramebufferObject()`: configure MSAA, initialize glad on first call
- `synchronize()`: copy CameraState from GLViewport, sync SceneGraph → GPU, write back previous pick/hover results
- `render()`: call pipeline.render(), perform pick/hover reads

- [ ] **Step 3: Implement GLViewport source**

Mouse event handling:
```
Ctrl + Left drag → Orbit
Shift + Left drag or Middle drag → Pan
Right drag → Zoom
Ctrl + Wheel → Zoom
Left click (no drag) → Pick
```

- [ ] **Step 4: Implement GLViewportRenderer source**

Key flow in `synchronize()`:
```cpp
void GLViewportRenderer::synchronize(QQuickFramebufferObject* item) {
    auto* viewport = static_cast<GLViewport*>(item);
    if (!ensureGladInitialized()) return;

    // Write back previous frame's pick/hover results
    if (m_lastPickResult.valid) {
        // ... emit signal via QMetaObject::invokeMethod
    }

    // Read new state from GUI thread
    auto camState = viewport->cameraState();
    float aspect = /* width / height */;
    m_frameState.viewMatrix = camState.viewMatrix();
    m_frameState.projMatrix = camState.projMatrix(aspect);
    m_frameState.cameraPos = camState.eyePosition();
    // ... copy other state

    // Sync scene data to GPU
    auto lock = m_scene.readLock();
    m_pipeline->synchronize(m_scene);
}
```

- [ ] **Step 5: Add glad and render deps to app CMakeLists.txt**

Add to app target link libraries:
```cmake
OpenGeoLab::Render
OpenGeoLab::Scene
Qt6::OpenGL
glad::glad
glm::glm
```

Add new source files and headers to `qt_add_executable` and `qt_add_qml_module` SOURCES.

- [ ] **Step 6: Build to verify**

```
cmake --build build --config RelWithDebInfo --parallel 8
```
Expected: PASS

- [ ] **Step 7: Commit**

```
git add src/app/include/opengeolab/app/gl_viewport.hpp src/app/include/opengeolab/app/gl_viewport_renderer.hpp src/app/src/gl_viewport.cpp src/app/src/gl_viewport_renderer.cpp src/app/CMakeLists.txt
git commit -m "feat(app): implement GLViewport and GLViewportRenderer

QQuickFramebufferObject integration with glad initialization on
render thread. Synchronize SceneGraph → GPU each frame. 4-pass
pipeline rendering with pick/hover result writeback.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 14: ViewportPanel.qml — embed GLViewport

**Files:**
- Modify: `src/app/resource/qml/sections/ViewportPanel.qml`

- [ ] **Step 1: Update ViewportPanel.qml to embed GLViewport**

Replace the current placeholder content with:
```qml
import QtQuick
import QtQuick.Layouts
import OpenGeoLab.App

Item {
    id: root

    GLViewport {
        id: viewport
        anchors.fill: parent
        pickingEnabled: true
        pickMode: 0 // VEF

        onEntityPicked: (shapeId, entityType, localId) => {
            console.log(qsTr("Picked: Shape %1, Type %2, Local %3")
                .arg(shapeId).arg(entityType).arg(localId))
        }

        onEntityHovered: (shapeId, entityType, localId) => {
            // Future: update status bar
        }

        onPickCleared: {
            // Future: clear selection UI
        }
    }
}
```

- [ ] **Step 2: Build to verify**

```
cmake --build build --config RelWithDebInfo --parallel 8
```
Expected: PASS

- [ ] **Step 3: Run full test suite**

```
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```
Expected: ALL PASS

- [ ] **Step 4: Commit**

```
git add src/app/resource/qml/sections/ViewportPanel.qml
git commit -m "feat(app): embed GLViewport in ViewportPanel.qml

Replace placeholder with interactive 3D viewport. Supports
orbit/pan/zoom via TrackballController, GPU picking with
entity picked/hovered signals to QML.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 15: Final integration build & full test run

- [ ] **Step 1: Full build**

```
cmake --build build --config RelWithDebInfo --parallel 8
```
Expected: Build succeeds with zero errors.

- [ ] **Step 2: Full test suite**

```
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```
Expected: ALL tests pass.

- [ ] **Step 3: Verify no compiler warnings in render module**

Check build output for warnings in `src/libs/render/` files.

- [ ] **Step 4: Manual smoke test**

Launch the app and verify:
1. 3D viewport renders (transparent background visible)
2. Create a box via the UI → appears rendered with faces and edges
3. Orbit (Ctrl+drag), pan (Shift+drag), zoom (wheel) work
4. Click on geometry → entityPicked signal fires
5. Hover → entityHovered signal fires

---

## Dependencies Graph

```
Task 0 (CMake skeleton)
   ├── Task 1 (ShaderProgram)
   ├── Task 2 (BatchUtils) [TDD, independent]
   ├── Task 3 (PickResolver) [TDD, independent]
   │
   ├── Task 4 (GpuBufferManager) ← needs Task 1
   ├── Task 5 (PickFbo) ← needs Task 1
   │
   ├── Task 6 (OpaquePass) ← needs Task 1, 4
   ├── Task 7 (WireframePass) ← needs Task 1, 4
   ├── Task 8 (HighlightPass) ← needs Task 1, 4, 2
   ├── Task 9 (SelectionPass) ← needs Task 1, 4, 5
   │
   ├── Task 10 (RenderPipeline) ← needs Tasks 2-9
   │
   ├── Task 11 (CameraState) [independent of render lib]
   ├── Task 12 (TrackballController) ← needs Task 11
   │
   ├── Task 13 (GLViewport + Renderer) ← needs Tasks 10, 11, 12
   ├── Task 14 (ViewportPanel.qml) ← needs Task 13
   │
   └── Task 15 (Integration) ← needs all
```

Parallelizable groups:
- **Group A** (render core): Tasks 1, 2, 3 can run in parallel after Task 0
- **Group B** (app camera): Tasks 11, 12 can run in parallel with render tasks
