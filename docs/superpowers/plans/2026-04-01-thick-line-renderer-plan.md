# ThickLineRenderer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace all `GL_LINES` + `glLineWidth()` usage with a TBO-based instanced quad renderer that works reliably in OpenGL 3.3 Core Profile, with per-pixel anti-aliasing.

**Architecture:** A new `ThickLineRenderer` utility class owned by `RenderPipeline` renders line segments as screen-space quads using Texture Buffer Objects (TBO) to zero-copy read position and index data from the existing VBO/IBO. Both `HighlightPass` and `WireframePass` delegate their edge drawing to this class.

**Tech Stack:** OpenGL 3.3 Core, GLAD, GLSL 330 core, GLM, TBO (GL_TEXTURE_BUFFER)

**Spec:** `docs/superpowers/specs/2026-04-01-thick-line-renderer-design.md`

---

## File Map

| File | Action | Responsibility |
|------|--------|---------------|
| `src/libs/render/src/core/thick_line_renderer.hpp` | CREATE | ThickLineRenderer class declaration |
| `src/libs/render/src/core/thick_line_renderer.cpp` | CREATE | Shader sources, TBO management, drawLines() |
| `src/libs/render/src/core/gpu_buffer_manager.hpp` | MODIFY | Add `mainVbo()` and `ibo()` getters |
| `src/libs/render/src/pass/highlight_pass.hpp` | MODIFY | Replace m_edgeShader with ThickLineRenderer* |
| `src/libs/render/src/pass/highlight_pass.cpp` | MODIFY | Replace edge GL_LINES with ThickLineRenderer; remove old edge shader sources and diagnostics |
| `src/libs/render/src/pass/wireframe_pass.hpp` | MODIFY | Add ThickLineRenderer* member |
| `src/libs/render/src/pass/wireframe_pass.cpp` | MODIFY | Replace edge GL_LINES with ThickLineRenderer; remove old line shader |
| `src/libs/render/src/render_pipeline.cpp` | MODIFY | Add ThickLineRenderer lifecycle + pass wiring; remove diagnostics |
| `src/libs/render/CMakeLists.txt` | MODIFY | Add thick_line_renderer.cpp to sources |
| `src/libs/core/include/opengeolab/core/color_map.hpp` | MODIFY | Restore lineWidth from 8.0 to OGL values (2.5/2.0) |
| `src/libs/core/test/color_map_test.cpp` | MODIFY | Update lineWidth assertions to match |
| `src/app/src/gl_viewport_renderer.cpp` | MODIFY | Remove diagnostic QDebug logging |

---

### Task 1: GpuBufferManager — Expose VBO/IBO Handles

**Files:**
- Modify: `src/libs/render/src/core/gpu_buffer_manager.hpp`

- [ ] **Step 1: Add mainVbo() and ibo() getters**

In `gpu_buffer_manager.hpp`, add two public getters after the existing `hasData()` declaration (before the `private:` section):

```cpp
    /** @brief Raw handle to the main interleaved VBO (pos+normal+color). */
    [[nodiscard]] GLuint mainVbo() const noexcept { return m_mainVbo; }

    /** @brief Raw handle to the shared index buffer (uint32_t). */
    [[nodiscard]] GLuint ibo() const noexcept { return m_ibo; }
```

- [ ] **Step 2: Build to verify**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 8`
Expected: BUILD SUCCESS

- [ ] **Step 3: Commit**

```
git add src/libs/render/src/core/gpu_buffer_manager.hpp
git commit -m "feat(render): expose VBO/IBO handles on GpuBufferManager

ThickLineRenderer needs direct GL buffer handles to create TBO views
for zero-copy position and index data access in the vertex shader.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 2: Create ThickLineRenderer

**Files:**
- Create: `src/libs/render/src/core/thick_line_renderer.hpp`
- Create: `src/libs/render/src/core/thick_line_renderer.cpp`
- Modify: `src/libs/render/CMakeLists.txt`

- [ ] **Step 1: Create thick_line_renderer.hpp**

Create `src/libs/render/src/core/thick_line_renderer.hpp`:

```cpp
/**
 * @file thick_line_renderer.hpp
 * @brief Screen-space thick line rendering via TBO-based instanced quads.
 *
 * Replaces glLineWidth (unreliable in Core Profile) with a vertex shader
 * that expands each GL_LINES segment into a screen-space quad with
 * per-pixel anti-aliasing.
 */

#pragma once

#include "shader_program.hpp"

#include <opengeolab/scene/render_mesh_data.hpp>

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief Renders line segments as screen-space quads with configurable width and AA.
 *
 * Uses Texture Buffer Objects to zero-copy read vertex positions and indices
 * from the existing GpuBufferManager VBO/IBO. Each line segment becomes 6
 * vertices (2 triangles) computed entirely in the vertex shader via gl_VertexID.
 *
 * Lifecycle: initialize() once → drawLines() per frame → cleanup() on shutdown.
 */
class ThickLineRenderer final {
public:
    /** @brief Parameters for a single thick-line draw batch. */
    struct DrawParams {
        GLuint positionVbo{0};  ///< Main VBO handle (interleaved RenderVertex, 40B stride).
        GLuint indexBuffer{0};  ///< IBO handle (uint32_t indices).
        glm::mat4 mvp{1.0F};   ///< Model-view-projection matrix.
        glm::vec2 viewport{};   ///< Viewport size in physical pixels (width×dpr, height×dpr).
        float lineWidth{1.5F};  ///< Line width in physical pixels.
        glm::vec4 color{};      ///< Override color (used when useVertexColor is false).
        bool useVertexColor{false}; ///< true: use original vertex color; false: use color uniform.
        float colorMix{0.0F};   ///< Blend factor: 0=pure vertex color, 1=pure override color.
        float depthBias{0.0005F}; ///< Clip-space depth offset towards camera.
    };

    void initialize();
    void cleanup();

    /**
     * @brief Draw edge segments from DrawRanges as screen-space thick quads.
     *
     * Caller must have a current GL context. This method manages its own
     * shader, TBO bindings, blend state, and empty VAO — the caller's
     * VAO binding is NOT required.
     */
    void drawLines(const DrawParams& params,
                   const std::vector<Scene::DrawRange>& ranges);

private:
    ShaderProgram m_shader;
    GLuint m_positionTbo{0}; ///< Texture for R32F view of position VBO.
    GLuint m_indexTbo{0};    ///< Texture for R32I view of index buffer.
    GLuint m_emptyVao{0};    ///< Empty VAO (Core Profile requires a bound VAO).
};

} // namespace OpenGeoLab::Render
```

- [ ] **Step 2: Create thick_line_renderer.cpp**

Create `src/libs/render/src/core/thick_line_renderer.cpp`:

```cpp
/**
 * @file thick_line_renderer.cpp
 * @brief ThickLineRenderer implementation — TBO-based screen-space thick lines
 */

#include "core/thick_line_renderer.hpp"

#include <string_view>

namespace OpenGeoLab::Render {

namespace {

// Each RenderVertex is 40 bytes = 10 floats: position[3] + normal[3] + color[4].
// The TBO uses GL_R32F format so each texel is one float.
constexpr int K_FLOATS_PER_VERTEX = 10;

constexpr std::string_view THICK_LINE_VS = R"glsl(
#version 330 core

uniform samplerBuffer u_positions;
uniform isamplerBuffer u_indices;
uniform int u_firstIndex;
uniform mat4 u_mvp;
uniform vec2 u_viewport;
uniform float u_lineWidth;
uniform float u_depthBias;

out float v_edgeDist;
out vec4 v_vertexColor;

void main() {
    int segId  = gl_VertexID / 6;
    int corner = gl_VertexID % 6;

    // Map 6 vertices to 4 quad corners (two triangles: 0-1-2, 2-1-3).
    //   corner: 0->BL  1->TL  2->BR  3->BR  4->TL  5->TR
    int qc;
    if      (corner == 0) qc = 0;
    else if (corner == 1) qc = 1;
    else if (corner == 2) qc = 2;
    else if (corner == 3) qc = 2;
    else if (corner == 4) qc = 1;
    else                  qc = 3;

    bool isP1  = (qc >= 2);
    float side = ((qc & 1) == 0) ? -1.0 : 1.0;

    // Fetch vertex indices from IBO TBO.
    int idx0 = texelFetch(u_indices, u_firstIndex + segId * 2    ).r;
    int idx1 = texelFetch(u_indices, u_firstIndex + segId * 2 + 1).r;

    // Fetch positions (stride = 10 floats per vertex).
    int b0 = idx0 * 10;
    int b1 = idx1 * 10;
    vec3 p0 = vec3(texelFetch(u_positions, b0    ).r,
                   texelFetch(u_positions, b0 + 1).r,
                   texelFetch(u_positions, b0 + 2).r);
    vec3 p1 = vec3(texelFetch(u_positions, b1    ).r,
                   texelFetch(u_positions, b1 + 1).r,
                   texelFetch(u_positions, b1 + 2).r);

    // Fetch vertex color (offset 6..9 in the vertex).
    int bc = isP1 ? b1 : b0;
    v_vertexColor = vec4(texelFetch(u_positions, bc + 6).r,
                         texelFetch(u_positions, bc + 7).r,
                         texelFetch(u_positions, bc + 8).r,
                         texelFetch(u_positions, bc + 9).r);

    // Project to clip space.
    vec4 clip0 = u_mvp * vec4(p0, 1.0);
    vec4 clip1 = u_mvp * vec4(p1, 1.0);

    // Screen-space direction and perpendicular.
    vec2 ndc0 = clip0.xy / clip0.w;
    vec2 ndc1 = clip1.xy / clip1.w;
    vec2 dirPx = (ndc1 - ndc0) * u_viewport * 0.5;
    float lenPx = length(dirPx);
    dirPx = (lenPx < 0.001) ? vec2(1.0, 0.0) : dirPx / lenPx;
    vec2 normPx = vec2(-dirPx.y, dirPx.x);

    // Half-width in pixels plus 1px AA margin.
    float hw = u_lineWidth * 0.5 + 1.0;
    vec2 offsetNDC = normPx * side * hw / (u_viewport * 0.5);

    // Final position with depth bias towards camera.
    vec4 baseClip = isP1 ? clip1 : clip0;
    gl_Position = vec4(baseClip.xy + offsetNDC * baseClip.w,
                       baseClip.z - u_depthBias * baseClip.w,
                       baseClip.w);

    v_edgeDist = side;
}
)glsl";

constexpr std::string_view THICK_LINE_FS = R"glsl(
#version 330 core

in float v_edgeDist;
in vec4 v_vertexColor;

uniform vec4 u_color;
uniform float u_lineWidth;
uniform bool u_useVertexColor;
uniform float u_colorMix;

out vec4 fragColor;

void main() {
    vec4 baseColor = u_useVertexColor
        ? mix(v_vertexColor, u_color, u_colorMix)
        : u_color;

    // Anti-aliasing: smoothstep at edge with 1px transition.
    float halfCore = u_lineWidth / (u_lineWidth + 2.0);
    float alpha = 1.0 - smoothstep(halfCore, 1.0, abs(v_edgeDist));

    fragColor = vec4(baseColor.rgb, baseColor.a * alpha);
}
)glsl";

} // namespace

void ThickLineRenderer::initialize() {
    m_shader.create(THICK_LINE_VS, THICK_LINE_FS);

    glGenTextures(1, &m_positionTbo);
    glGenTextures(1, &m_indexTbo);
    glGenVertexArrays(1, &m_emptyVao);
}

void ThickLineRenderer::cleanup() {
    if(m_emptyVao != 0) {
        glDeleteVertexArrays(1, &m_emptyVao);
        m_emptyVao = 0;
    }
    if(m_indexTbo != 0) {
        glDeleteTextures(1, &m_indexTbo);
        m_indexTbo = 0;
    }
    if(m_positionTbo != 0) {
        glDeleteTextures(1, &m_positionTbo);
        m_positionTbo = 0;
    }
    m_shader.destroy();
}

void ThickLineRenderer::drawLines(const DrawParams& params,
                                  const std::vector<Scene::DrawRange>& ranges) {
    if(ranges.empty() || params.positionVbo == 0 || params.indexBuffer == 0) {
        return;
    }

    // Bind TBOs: zero-copy views into existing VBO and IBO.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, m_positionTbo);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, params.positionVbo);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, m_indexTbo);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, params.indexBuffer);

    // Shader setup.
    m_shader.use();
    m_shader.setInt("u_positions", 0);
    m_shader.setInt("u_indices", 1);
    m_shader.setMat4("u_mvp", params.mvp);
    m_shader.setVec4("u_color", params.color);
    m_shader.setFloat("u_lineWidth", params.lineWidth);
    m_shader.setFloat("u_depthBias", params.depthBias);
    m_shader.setFloat("u_colorMix", params.colorMix);

    const GLint vp_loc = glGetUniformLocation(m_shader.id(), "u_viewport");
    glUniform2f(vp_loc, params.viewport.x, params.viewport.y);

    const GLint uvc_loc = glGetUniformLocation(m_shader.id(), "u_useVertexColor");
    glUniform1i(uvc_loc, params.useVertexColor ? 1 : 0);

    // State: blending for AA edges, depth write on.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Bind empty VAO (Core Profile requires a bound VAO).
    glBindVertexArray(m_emptyVao);

    // Draw each range as instanced quads.
    for(const auto& r : ranges) {
        const auto num_segments = r.indexCount / 2;
        if(num_segments == 0) {
            continue;
        }
        m_shader.setInt("u_firstIndex", static_cast<int>(r.indexOffset));
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(num_segments * 6));
    }

    // Restore state.
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
}

} // namespace OpenGeoLab::Render
```

- [ ] **Step 3: Register in CMakeLists.txt**

In `src/libs/render/CMakeLists.txt`, add `src/core/thick_line_renderer.cpp` to the `render_sources` list, after `src/core/gpu_buffer_manager.cpp`:

```cmake
set(render_sources
    src/render_pipeline.cpp
    src/batch_utils.cpp
    src/pick_resolver.cpp
    src/core/shader_program.cpp
    src/core/gpu_buffer_manager.cpp
    src/core/thick_line_renderer.cpp
    src/core/pick_fbo.cpp
    src/pass/opaque_pass.cpp
    src/pass/wireframe_pass.cpp
    src/pass/highlight_pass.cpp
    src/pass/selection_pass.cpp)
```

- [ ] **Step 4: Build to verify**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 8`
Expected: BUILD SUCCESS (ThickLineRenderer compiles and links into render DLL)

- [ ] **Step 5: Commit**

```
git add src/libs/render/src/core/thick_line_renderer.hpp \
        src/libs/render/src/core/thick_line_renderer.cpp \
        src/libs/render/CMakeLists.txt
git commit -m "feat(render): add ThickLineRenderer with TBO-based screen-space quads

Renders line segments as screen-space quads using Texture Buffer Objects
for zero-copy VBO/IBO access. Each segment becomes 6 vertices (2 tris)
expanded in the vertex shader via gl_VertexID. Includes 1px smoothstep
anti-aliasing and configurable depth bias.

Replaces glLineWidth() which is unreliable in OpenGL 3.3 Core Profile.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 3: Wire ThickLineRenderer into RenderPipeline

**Files:**
- Modify: `src/libs/render/src/render_pipeline.cpp`

- [ ] **Step 1: Add include and member**

At the top of `render_pipeline.cpp`, add the include after the existing pass includes:

```cpp
#include "core/thick_line_renderer.hpp"
```

In `struct RenderPipeline::Impl`, add the member after `selectionPass`:

```cpp
struct RenderPipeline::Impl {
    GpuBufferManager bufferManager;
    OpaquePass opaquePass;
    WireframePass wireframePass;
    HighlightPass highlightPass;
    SelectionPass selectionPass;
    ThickLineRenderer thickLineRenderer;
    std::unique_ptr<Scene::TopologyIndex> topologyIndex;
    std::unique_ptr<PickResolver> pickResolver;
    bool initialized{false};
};
```

- [ ] **Step 2: Add lifecycle calls**

In `RenderPipeline::initialize()`, add after `m_impl->selectionPass.initialize();`:

```cpp
    m_impl->thickLineRenderer.initialize();
```

In `RenderPipeline::cleanup()`, add before `m_impl->selectionPass.cleanup();`:

```cpp
    m_impl->thickLineRenderer.cleanup();
```

- [ ] **Step 3: Wire to passes**

In `RenderPipeline::initialize()`, after all passes are initialized, add:

```cpp
    m_impl->wireframePass.setThickLineRenderer(&m_impl->thickLineRenderer);
    m_impl->highlightPass.setThickLineRenderer(&m_impl->thickLineRenderer);
```

(These methods will be created in Tasks 4 and 5.)

- [ ] **Step 4: Build to verify**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 8`
Expected: BUILD FAILURE (setThickLineRenderer not yet declared — expected, will be resolved in Tasks 4-5)

Note: This task's commit is deferred to after Task 5 when all pieces compile together.

---

### Task 4: Integrate ThickLineRenderer into HighlightPass

**Files:**
- Modify: `src/libs/render/src/pass/highlight_pass.hpp`
- Modify: `src/libs/render/src/pass/highlight_pass.cpp`

- [ ] **Step 1: Update highlight_pass.hpp**

Replace the full file content with:

```cpp
/**
 * @file highlight_pass.hpp
 * @brief Redraws selected/hovered geometry with highlight color overlay
 */

#pragma once

#include "core/shader_program.hpp"
#include "render_pass_base.hpp"

namespace OpenGeoLab::Render {

class ThickLineRenderer;

class HighlightPass final : public RenderPassBase {
public:
    void render(const FrameState& state, const GpuBufferManager& buffers) override;

    /** @brief Set the shared thick-line renderer (owned by RenderPipeline). */
    void setThickLineRenderer(ThickLineRenderer* renderer) { m_thickLine = renderer; }

protected:
    bool onInitialize() override;
    void onCleanup() override;

private:
    ShaderProgram m_faceShader;  /**< Lit faces with highlight color mix */
    ShaderProgram m_pointShader; /**< Flat-color points with highlight */
    ThickLineRenderer* m_thickLine{nullptr};
};

} // namespace OpenGeoLab::Render
```

Key change: removed `m_edgeShader`, added `m_thickLine` pointer and setter.

- [ ] **Step 2: Update highlight_pass.cpp**

Replace the full file content with the version below. Key changes:
- Removed `HIGHLIGHT_EDGE_VS`, `HIGHLIGHT_EDGE_FS` shader sources
- Removed `m_edgeShader` creation/destruction
- Removed `#include <cstdio>` and all `std::fprintf` diagnostic lines
- Edge sections now use `m_thickLine->drawLines()` instead of `glLineWidth` + `multiDrawElements(GL_LINES)`

```cpp
#include "pass/highlight_pass.hpp"

#include "core/gpu_buffer_manager.hpp"
#include "core/thick_line_renderer.hpp"

#include <opengeolab/core/color_map.hpp>
#include <opengeolab/render/batch_utils.hpp>

#include <glad/gl.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string_view>

namespace OpenGeoLab::Render {

namespace {

// --- Shaders (face + point only; edges use ThickLineRenderer) ---

constexpr std::string_view HIGHLIGHT_FACE_VS = R"glsl(
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
)glsl";

constexpr std::string_view HIGHLIGHT_FACE_FS = R"glsl(
#version 330 core
in vec3 v_normal;
in vec4 v_color;
in vec3 v_viewPos;
uniform vec4 u_highlightColor;
uniform float u_alpha;
out vec4 fragColor;
void main() {
    vec3 N = normalize(v_normal);
    vec3 V = normalize(-v_viewPos);
    float ambient      = 0.35;
    float headlamp     = abs(dot(N, V));
    float skyLight     = max(dot(N, vec3(0.0, 1.0, 0.0)), 0.0) * 0.15;
    float groundBounce = max(dot(N, vec3(0.0, -1.0, 0.0)), 0.0) * 0.05;
    float lighting     = ambient + headlamp * 0.55 + skyLight + groundBounce;
    vec3 litColor = v_color.rgb * min(lighting, 1.0);
    vec3 finalColor = mix(litColor, u_highlightColor.rgb, u_highlightColor.a);
    fragColor = vec4(finalColor * u_alpha, u_alpha);
}
)glsl";

constexpr std::string_view HIGHLIGHT_POINT_VS = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 2) in vec4 a_color;
uniform mat4 u_mvp;
uniform float u_pointSize;
out vec4 v_color;
void main() {
    vec4 pos = u_mvp * vec4(a_position, 1.0);
    pos.z -= 0.0005 * pos.w;
    gl_Position = pos;
    gl_PointSize = u_pointSize;
    v_color = a_color;
}
)glsl";

constexpr std::string_view HIGHLIGHT_POINT_FS = R"glsl(
#version 330 core
in vec4 v_color;
uniform vec4 u_highlightColor;
uniform float u_alpha;
out vec4 fragColor;
void main() {
    vec3 finalColor = mix(v_color.rgb, u_highlightColor.rgb, u_highlightColor.a);
    fragColor = vec4(finalColor, u_alpha);
}
)glsl";

/// Bundled transform matrices for face highlight rendering.
struct FaceTransforms {
    glm::mat4 mvp;
    glm::mat4 modelView;
    glm::mat3 normalMatrix;
};

/// Convert Core::RenderColor to glm::vec4.
glm::vec4 toVec4(const Core::RenderColor& c) { return {c.r, c.g, c.b, c.a}; }

/// Check whether an entity type represents face/solid geometry.
bool isFaceType(Core::EntityType t) {
    return t == Core::EntityType::GeoFace || t == Core::EntityType::GeoSolid;
}

/// Check whether an entity type represents edge/wire geometry.
bool isEdgeType(Core::EntityType t) {
    return t == Core::EntityType::GeoEdge || t == Core::EntityType::GeoWire;
}

/// Check whether an entity type represents vertex geometry.
bool isVertexType(Core::EntityType t) { return t == Core::EntityType::GeoVertex; }

/// Partition highlight entries into face, edge, and vertex range lists.
struct PartitionedRanges {
    std::vector<Scene::DrawRange> faces;
    std::vector<Scene::DrawRange> edges;
    std::vector<Scene::DrawRange> vertices;
};

PartitionedRanges partition(const std::vector<HighlightEntry>& entries) {
    PartitionedRanges out;
    for(const auto& e : entries) {
        if(isFaceType(e.entityType)) {
            out.faces.push_back(e.range);
        } else if(isEdgeType(e.entityType)) {
            out.edges.push_back(e.range);
        } else if(isVertexType(e.entityType)) {
            out.vertices.push_back(e.range);
        }
    }
    return out;
}

} // namespace

bool HighlightPass::onInitialize() {
    if(!m_faceShader.create(HIGHLIGHT_FACE_VS, HIGHLIGHT_FACE_FS)) {
        return false;
    }
    if(!m_pointShader.create(HIGHLIGHT_POINT_VS, HIGHLIGHT_POINT_FS)) {
        m_faceShader.destroy();
        return false;
    }
    return true;
}

void HighlightPass::onCleanup() {
    m_pointShader.destroy();
    m_faceShader.destroy();
}

void HighlightPass::render(const FrameState& state, const GpuBufferManager& buffers) {
    if((state.selectedEntries.empty() && state.hoveredEntries.empty()) || !buffers.hasData()) {
        return;
    }

    const auto& cfg = Core::ColorMap::active();
    const glm::mat4 mvp = state.projMatrix * state.viewMatrix;
    const glm::mat3 normal_matrix = glm::inverseTranspose(glm::mat3(state.viewMatrix));
    const FaceTransforms transforms{mvp, state.viewMatrix, normal_matrix};
    constexpr float alpha = 1.0F;

    const glm::vec2 viewport{
        static_cast<float>(state.viewportWidth) * state.devicePixelRatio,
        static_cast<float>(state.viewportHeight) * state.devicePixelRatio};

    buffers.bindMainVao();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // --- Selection highlight ---
    {
        auto [faces, edges, vertices] = partition(state.selectedEntries);

        if(!faces.empty()) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0F, 5.0F);
            m_faceShader.use();
            m_faceShader.setMat4("u_mvp", transforms.mvp);
            m_faceShader.setMat4("u_modelView", transforms.modelView);
            const GLint loc = glGetUniformLocation(m_faceShader.id(), "u_normalMatrix");
            glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(transforms.normalMatrix));
            m_faceShader.setVec4("u_highlightColor", toVec4(cfg.selectionFace.color));
            m_faceShader.setFloat("u_alpha", alpha);
            const auto batch = BatchUtils::buildIndexedBatch(
                faces, [](const Scene::DrawRange&) { return true; });
            BatchUtils::multiDrawElements(GL_TRIANGLES, batch);
            glDisable(GL_POLYGON_OFFSET_FILL);
        }

        if(!edges.empty() && m_thickLine != nullptr) {
            buffers.unbind();
            m_thickLine->drawLines(
                {.positionVbo = buffers.mainVbo(),
                 .indexBuffer = buffers.ibo(),
                 .mvp = mvp,
                 .viewport = viewport,
                 .lineWidth = cfg.selectionEdgeVertex.lineWidth * state.devicePixelRatio,
                 .color = toVec4(cfg.selectionEdgeVertex.color),
                 .useVertexColor = false,
                 .colorMix = 0.0F,
                 .depthBias = 0.0005F},
                edges);
            buffers.bindMainVao();
        }

        if(!vertices.empty()) {
            const float pt_size = cfg.defaultPointSize * cfg.selectionEdgeVertex.pointScale;
            glEnable(GL_PROGRAM_POINT_SIZE);
            m_pointShader.use();
            m_pointShader.setMat4("u_mvp", mvp);
            m_pointShader.setFloat("u_pointSize", pt_size);
            m_pointShader.setVec4("u_highlightColor", toVec4(cfg.selectionEdgeVertex.color));
            m_pointShader.setFloat("u_alpha", alpha);
            const auto batch = BatchUtils::buildArrayBatch(
                vertices, [](const Scene::DrawRange&) { return true; });
            BatchUtils::multiDrawArrays(GL_POINTS, batch);
            glDisable(GL_PROGRAM_POINT_SIZE);
        }
    }

    // --- Hover highlight ---
    {
        auto [faces, edges, vertices] = partition(state.hoveredEntries);

        if(!faces.empty()) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0F, 5.0F);
            m_faceShader.use();
            m_faceShader.setMat4("u_mvp", transforms.mvp);
            m_faceShader.setMat4("u_modelView", transforms.modelView);
            const GLint loc = glGetUniformLocation(m_faceShader.id(), "u_normalMatrix");
            glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(transforms.normalMatrix));
            m_faceShader.setVec4("u_highlightColor", toVec4(cfg.hoverFace.color));
            m_faceShader.setFloat("u_alpha", alpha);
            const auto batch = BatchUtils::buildIndexedBatch(
                faces, [](const Scene::DrawRange&) { return true; });
            BatchUtils::multiDrawElements(GL_TRIANGLES, batch);
            glDisable(GL_POLYGON_OFFSET_FILL);
        }

        if(!edges.empty() && m_thickLine != nullptr) {
            buffers.unbind();
            m_thickLine->drawLines(
                {.positionVbo = buffers.mainVbo(),
                 .indexBuffer = buffers.ibo(),
                 .mvp = mvp,
                 .viewport = viewport,
                 .lineWidth = cfg.hoverEdgeVertex.lineWidth * state.devicePixelRatio,
                 .color = toVec4(cfg.hoverEdgeVertex.color),
                 .useVertexColor = false,
                 .colorMix = 0.0F,
                 .depthBias = 0.0005F},
                edges);
            buffers.bindMainVao();
        }

        if(!vertices.empty()) {
            const float pt_size = cfg.defaultPointSize * cfg.hoverEdgeVertex.pointScale;
            glEnable(GL_PROGRAM_POINT_SIZE);
            m_pointShader.use();
            m_pointShader.setMat4("u_mvp", mvp);
            m_pointShader.setFloat("u_pointSize", pt_size);
            m_pointShader.setVec4("u_highlightColor", toVec4(cfg.hoverEdgeVertex.color));
            m_pointShader.setFloat("u_alpha", alpha);
            const auto batch = BatchUtils::buildArrayBatch(
                vertices, [](const Scene::DrawRange&) { return true; });
            BatchUtils::multiDrawArrays(GL_POINTS, batch);
            glDisable(GL_PROGRAM_POINT_SIZE);
        }
    }

    glDepthFunc(GL_LESS);
    buffers.unbind();
}

} // namespace OpenGeoLab::Render
```

- [ ] **Step 3: Build to verify**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 8`
Expected: May still fail until Task 5 adds the WireframePass setter. Continue to Task 5.

---

### Task 5: Integrate ThickLineRenderer into WireframePass

**Files:**
- Modify: `src/libs/render/src/pass/wireframe_pass.hpp`
- Modify: `src/libs/render/src/pass/wireframe_pass.cpp`

- [ ] **Step 1: Update wireframe_pass.hpp**

Replace the full file content with:

```cpp
/**
 * @file wireframe_pass.hpp
 * @brief Draws edges (thick quads) and vertex points (GL_POINTS)
 */

#pragma once

#include "core/shader_program.hpp"
#include "render_pass_base.hpp"

namespace OpenGeoLab::Render {

class ThickLineRenderer;

class WireframePass final : public RenderPassBase {
public:
    void render(const FrameState& state, const GpuBufferManager& buffers) override;

    /** @brief Set the shared thick-line renderer (owned by RenderPipeline). */
    void setThickLineRenderer(ThickLineRenderer* renderer) { m_thickLine = renderer; }

protected:
    bool onInitialize() override;
    void onCleanup() override;

private:
    ShaderProgram m_pointShader;
    ThickLineRenderer* m_thickLine{nullptr};
};

} // namespace OpenGeoLab::Render
```

Key change: removed `m_lineShader`, added `m_thickLine` pointer.

- [ ] **Step 2: Update wireframe_pass.cpp**

Replace the full file content with:

```cpp
#include "pass/wireframe_pass.hpp"

#include "core/gpu_buffer_manager.hpp"
#include "core/thick_line_renderer.hpp"

#include <opengeolab/core/color_map.hpp>
#include <opengeolab/render/batch_utils.hpp>
#include <opengeolab/scene/display_mode.hpp>

#include <glad/gl.h>

#include <string_view>

namespace OpenGeoLab::Render {

namespace {

constexpr std::string_view POINT_VS = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 2) in vec4 a_color;
uniform mat4 u_mvp;
uniform float u_pointSize;
out vec4 v_color;
void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    gl_PointSize = u_pointSize;
    v_color = a_color;
}
)glsl";

constexpr std::string_view WIREFRAME_FS = R"glsl(
#version 330 core
in vec4 v_color;
uniform float u_alpha;
out vec4 fragColor;
void main() {
    float a = v_color.a * u_alpha;
    fragColor = vec4(v_color.rgb * a, a);
}
)glsl";

constexpr float POINT_SIZE = 6.0F;

} // namespace

bool WireframePass::onInitialize() {
    return m_pointShader.create(POINT_VS, WIREFRAME_FS);
}

void WireframePass::onCleanup() { m_pointShader.destroy(); }

void WireframePass::render(const FrameState& state, const GpuBufferManager& buffers) {
    if(!buffers.hasData()) {
        return;
    }

    using Scene::DisplayModeMask;
    if((state.displayMask & DisplayModeMask::Wireframe) == DisplayModeMask::None) {
        return;
    }

    const auto& cfg = Core::ColorMap::active();
    const glm::mat4 mvp = state.projMatrix * state.viewMatrix;
    constexpr float alpha = 1.0F;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // --- Edges via ThickLineRenderer ---
    if(m_thickLine != nullptr) {
        const glm::vec2 viewport{
            static_cast<float>(state.viewportWidth) * state.devicePixelRatio,
            static_cast<float>(state.viewportHeight) * state.devicePixelRatio};
        m_thickLine->drawLines(
            {.positionVbo = buffers.mainVbo(),
             .indexBuffer = buffers.ibo(),
             .mvp = mvp,
             .viewport = viewport,
             .lineWidth = cfg.defaultEdgeWidth * state.devicePixelRatio,
             .color = {},
             .useVertexColor = true,
             .colorMix = 0.0F,
             .depthBias = 0.0005F},
            buffers.lineRanges());
    }

    // --- Points via standard shader ---
    buffers.bindMainVao();
    glEnable(GL_PROGRAM_POINT_SIZE);

    m_pointShader.use();
    m_pointShader.setMat4("u_mvp", mvp);
    m_pointShader.setFloat("u_alpha", alpha);
    m_pointShader.setFloat("u_pointSize", POINT_SIZE);
    const auto point_batch = BatchUtils::buildArrayBatch(
        buffers.pointRanges(), [](const Scene::DrawRange&) { return true; });
    BatchUtils::multiDrawArrays(GL_POINTS, point_batch);

    buffers.unbind();
    glDisable(GL_PROGRAM_POINT_SIZE);
    glDepthFunc(GL_LESS);
}

} // namespace OpenGeoLab::Render
```

- [ ] **Step 3: Build to verify**

Run: `cmake --build build --config RelWithDebInfo --parallel 8`
Expected: FULL BUILD SUCCESS (all passes + pipeline compile together)

- [ ] **Step 4: Run tests**

Run: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
Expected: All 28 tests pass

- [ ] **Step 5: Commit Tasks 3-5 together**

```
git add src/libs/render/src/render_pipeline.cpp \
        src/libs/render/src/pass/highlight_pass.hpp \
        src/libs/render/src/pass/highlight_pass.cpp \
        src/libs/render/src/pass/wireframe_pass.hpp \
        src/libs/render/src/pass/wireframe_pass.cpp
git commit -m "feat(render): integrate ThickLineRenderer into HighlightPass and WireframePass

Both passes now delegate edge drawing to ThickLineRenderer instead of
using glLineWidth + GL_LINES. This gives correct thick-line rendering
in OpenGL 3.3 Core Profile where glLineWidth(>1.0) is unreliable.

HighlightPass: removed old edge shader; edges use highlight color overlay.
WireframePass: removed old line shader; edges use original vertex color.
RenderPipeline: owns ThickLineRenderer, passes reference to both passes.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 6: Restore Values and Remove Diagnostics

**Files:**
- Modify: `src/libs/core/include/opengeolab/core/color_map.hpp`
- Modify: `src/libs/core/test/color_map_test.cpp`
- Modify: `src/libs/render/src/render_pipeline.cpp`
- Modify: `src/app/src/gl_viewport_renderer.cpp`

- [ ] **Step 1: Restore lineWidth in color_map.hpp**

In `color_map.hpp`, change the `kDefault` hover and selection lineWidth from `8.F` to OGL reference values:

```cpp
    // hoverEdgeVertex: orange #ff7f00
    {.color = {1.F, 0.498F, 0.F, 1.F}, .lineWidth = 2.5F, .pointScale = 1.5F},
```

```cpp
    // selectionEdgeVertex: red-pink #ff165d
    {.color = {1.F, 0.086F, 0.365F, 1.F}, .lineWidth = 2.0F, .pointScale = 1.2F},
```

- [ ] **Step 2: Update color_map_test.cpp**

Change the two lineWidth assertions from `8.0f` to match:

```cpp
    TEST_CASE("kDefault hover edge/vertex color is orange #ff7f00") {
        // ...
        CHECK(style.lineWidth == doctest::Approx(2.5f));
        // ...
    }

    TEST_CASE("kDefault selection edge/vertex color is red-pink #ff165d") {
        // ...
        CHECK(style.lineWidth == doctest::Approx(2.0f));
        // ...
    }
```

- [ ] **Step 3: Remove diagnostics from render_pipeline.cpp**

1. Remove the `#include <cstdio>` line.
2. Remove the `GL_LINE_WIDTH_RANGE` diagnostic block from `initialize()`:
   ```cpp
   // REMOVE this entire block:
   {
       GLfloat range[2] = {0.F, 0.F};
       glGetFloatv(GL_LINE_WIDTH_RANGE, range);
       std::fprintf(stderr, "[RenderPipeline] GL_LINE_WIDTH_RANGE: [%.1f, %.1f]\n",
                    static_cast<double>(range[0]), static_cast<double>(range[1]));
   }
   ```

- [ ] **Step 4: Remove diagnostics from gl_viewport_renderer.cpp**

Search for and remove:
1. Any `#include <QDebug>` line
2. Any `qDebug() << ...` lines (diagnostic logging added during bugfixing)

- [ ] **Step 5: Build and test**

Run: `cmake --build build --config RelWithDebInfo --parallel 8 && ctest --test-dir build -C RelWithDebInfo --output-on-failure`
Expected: BUILD SUCCESS, all 28 tests pass

- [ ] **Step 6: Commit**

```
git add src/libs/core/include/opengeolab/core/color_map.hpp \
        src/libs/core/test/color_map_test.cpp \
        src/libs/render/src/render_pipeline.cpp \
        src/app/src/gl_viewport_renderer.cpp
git commit -m "chore(render): restore OGL line widths and remove diagnostic logging

Restore hover lineWidth to 2.5px and selection to 2.0px (OGL reference).
Remove all fprintf/qDebug diagnostic logging added during development.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 7: Visual Verification

- [ ] **Step 1: Full rebuild**

Run: `cmake --build build --config RelWithDebInfo --parallel 8`
Expected: BUILD SUCCESS, clean output

- [ ] **Step 2: Run all tests**

Run: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
Expected: All 28 tests pass

- [ ] **Step 3: Visual verification**

Launch `build\bin\opengeolab_app.exe` and verify:
1. Load a model with edges and faces
2. **Hover edge**: orange thick line (~2.5px) with smooth AA edges
3. **Select edge**: red-pink thick line (~2.0px) with smooth AA edges
4. **Wireframe edges**: golden-yellow lines (~1.5px) with AA
5. **Hover vertex**: blue enlarged point (unchanged)
6. **Select face**: blue overlay (unchanged)
7. **Solid selection**: all contained VEF highlighted (unchanged)
8. **Rotate/zoom**: no visual artifacts, smooth performance

- [ ] **Step 4: Final commit (all remaining changes)**

If any files have uncommitted changes from earlier tasks, stage and commit them now.
