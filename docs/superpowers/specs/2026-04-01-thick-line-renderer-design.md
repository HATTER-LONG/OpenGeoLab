# Thick Line Renderer Design Spec

## Problem

OpenGeoLab uses OpenGL 3.3 **Core Profile** (required by Qt 6 QML). In Core Profile,
`glLineWidth(>1.0)` is not guaranteed — many drivers clamp to 1.0. This means
highlight (hover/selection) edge enlargement and wireframe edge width control
do not work. OGL (the reference project) uses Compatibility Profile where
`glLineWidth` works, but switching profiles breaks Qt 6 UI rendering.

## Approach

Replace all `GL_LINES` + `glLineWidth` usage with a **TBO-based instanced quad**
technique: each line segment is rendered as a screen-space quad (2 triangles)
whose width is precisely controlled in the vertex shader.

A new `ThickLineRenderer` utility class encapsulates the shader, TBO management,
and draw logic. Both `HighlightPass` and `WireframePass` use it.

## Scope

- **In scope**: HighlightPass edges, WireframePass edges, anti-aliasing, DPI scaling
- **Out of scope**: Round end caps, miter/bevel joins, world-space width, point rendering

## Architecture

```
RenderPipeline
  ├── m_opaquePass
  ├── m_wireframePass ────── uses ──→ m_thickLineRenderer (ref)
  ├── m_highlightPass ────── uses ──→ m_thickLineRenderer (ref)
  ├── m_selectionPass
  └── m_thickLineRenderer    ← owner
```

`ThickLineRenderer` is a **stateless utility** — no frame-to-frame state. It owns:
- One `ShaderProgram` (THICK_LINE_VS + THICK_LINE_FS)
- Two `GLuint` texture objects (TBO views into VBO and IBO)
- One empty `GLuint` VAO (Core Profile requires a bound VAO for draw calls)

## ThickLineRenderer API

```cpp
/// @file thick_line_renderer.hpp
class ThickLineRenderer final {
public:
    void initialize();
    void cleanup();

    struct DrawParams {
        GLuint positionVbo;      ///< Main VBO (interleaved RenderVertex, 40B stride)
        GLuint indexBuffer;      ///< Index buffer (uint32_t)
        glm::mat4 mvp;          ///< Model-view-projection matrix
        glm::vec2 viewport;     ///< Viewport size in physical pixels (w × dpr, h × dpr)
        float lineWidth;        ///< Line width in physical pixels
        glm::vec4 color;        ///< Override color (used when useVertexColor=false)
        bool useVertexColor;    ///< true: use original vertex color; false: use color uniform
        float colorMix;         ///< 0=pure vertex color, 1=pure override color (when useVertexColor=true)
        float depthBias;        ///< Clip-space depth offset (default 0.0005)
    };

    /// Draw edge segments from the given DrawRanges as screen-space thick quads.
    void drawLines(const DrawParams& params,
                   const std::vector<Scene::DrawRange>& ranges);

private:
    ShaderProgram m_shader;
    GLuint m_positionTbo{0};
    GLuint m_indexTbo{0};
    GLuint m_emptyVao{0};
};
```

## GpuBufferManager Changes

Add two public getters (no other changes):

```cpp
[[nodiscard]] GLuint mainVbo() const noexcept { return m_mainVbo; }
[[nodiscard]] GLuint ibo() const noexcept { return m_ibo; }
```

## Vertex Shader (THICK_LINE_VS)

```glsl
#version 330 core

uniform samplerBuffer u_positions;    // GL_R32F view of VBO (stride=10 floats per vertex)
uniform isamplerBuffer u_indices;     // GL_R32I view of IBO
uniform int u_firstIndex;            // DrawRange::indexOffset
uniform mat4 u_mvp;
uniform vec2 u_viewport;             // Physical pixel dimensions
uniform float u_lineWidth;           // Physical pixel width
uniform float u_depthBias;           // Clip-space depth bias

out float v_edgeDist;     // [-1, +1] across line width for AA
out vec4 v_vertexColor;   // Original vertex color

void main() {
    // 1. Locate segment and quad corner from gl_VertexID
    int segId  = gl_VertexID / 6;
    int corner = gl_VertexID % 6;

    // Two triangles per segment: vertices (0,1,2) and (2,1,3)
    // corner → quadCorner mapping: 0→0(BL) 1→1(TL) 2→2(BR) 3→2(BR) 4→1(TL) 5→3(TR)
    int quadCorner;
    if      (corner == 0) quadCorner = 0;
    else if (corner == 1) quadCorner = 1;
    else if (corner == 2) quadCorner = 2;
    else if (corner == 3) quadCorner = 2;
    else if (corner == 4) quadCorner = 1;
    else                  quadCorner = 3;

    bool isP1  = (quadCorner >= 2);                        // Which endpoint
    float side = ((quadCorner & 1) == 0) ? -1.0 : 1.0;    // Offset direction

    // 2. Fetch indices from IBO TBO
    int idx0 = texelFetch(u_indices, u_firstIndex + segId * 2    ).r;
    int idx1 = texelFetch(u_indices, u_firstIndex + segId * 2 + 1).r;

    // 3. Fetch positions from VBO TBO (stride = 10 floats = 40 bytes per vertex)
    int b0 = idx0 * 10, b1 = idx1 * 10;
    vec3 p0 = vec3(texelFetch(u_positions, b0  ).r,
                   texelFetch(u_positions, b0+1).r,
                   texelFetch(u_positions, b0+2).r);
    vec3 p1 = vec3(texelFetch(u_positions, b1  ).r,
                   texelFetch(u_positions, b1+1).r,
                   texelFetch(u_positions, b1+2).r);

    // 4. Fetch vertex color (floats at offset 6-9 in the vertex)
    int bc = isP1 ? b1 : b0;
    v_vertexColor = vec4(texelFetch(u_positions, bc+6).r,
                         texelFetch(u_positions, bc+7).r,
                         texelFetch(u_positions, bc+8).r,
                         texelFetch(u_positions, bc+9).r);

    // 5. Project to clip space
    vec4 clip0 = u_mvp * vec4(p0, 1.0);
    vec4 clip1 = u_mvp * vec4(p1, 1.0);

    // 6. Compute screen-space line direction and perpendicular
    vec2 ndc0 = clip0.xy / clip0.w;
    vec2 ndc1 = clip1.xy / clip1.w;
    vec2 dirPx = (ndc1 - ndc0) * u_viewport * 0.5;
    float lenPx = length(dirPx);
    dirPx = (lenPx < 0.001) ? vec2(1.0, 0.0) : dirPx / lenPx;
    vec2 normPx = vec2(-dirPx.y, dirPx.x);

    // 7. Compute offset: half-width + 1px AA margin
    float hw = u_lineWidth * 0.5 + 1.0;
    vec2 offsetNDC = normPx * side * hw / (u_viewport * 0.5);

    // 8. Final position with depth bias
    vec4 baseClip = isP1 ? clip1 : clip0;
    gl_Position = vec4(baseClip.xy + offsetNDC * baseClip.w,
                       baseClip.z - u_depthBias * baseClip.w,
                       baseClip.w);

    v_edgeDist = side;
}
```

## Fragment Shader (THICK_LINE_FS)

```glsl
#version 330 core

in float v_edgeDist;
in vec4 v_vertexColor;

uniform vec4 u_color;
uniform float u_lineWidth;
uniform bool u_useVertexColor;
uniform float u_colorMix;

out vec4 fragColor;

void main() {
    // Choose color source
    vec4 baseColor = u_useVertexColor
        ? mix(v_vertexColor, u_color, u_colorMix)
        : u_color;

    // Anti-aliasing: smoothstep at edge (1px transition)
    float halfCore = u_lineWidth / (u_lineWidth + 2.0);
    float alpha = 1.0 - smoothstep(halfCore, 1.0, abs(v_edgeDist));

    fragColor = vec4(baseColor.rgb, baseColor.a * alpha);
}
```

## TBO Binding Flow

Per-frame, before drawing edges:

```cpp
// Bind position TBO → GL_R32F view of interleaved VBO
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_BUFFER, m_positionTbo);
glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, params.positionVbo);

// Bind index TBO → GL_R32I view of IBO
glActiveTexture(GL_TEXTURE1);
glBindTexture(GL_TEXTURE_BUFFER, m_indexTbo);
glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, params.indexBuffer);
```

`glTexBuffer` does NOT copy data — it creates a view of the existing buffer.
Cost is negligible (a GL state change).

## Draw Call Structure

For each `DrawRange` in the edge list:

```cpp
m_shader.setInt("u_firstIndex", static_cast<int>(range.indexOffset));
uint32_t numSegments = range.indexCount / 2;
glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(numSegments * 6));
```

An empty VAO must be bound (Core Profile requirement) but no vertex attributes
are consumed — all data comes from TBO `texelFetch`.

## Depth and Blending

| Property | Value | Rationale |
|----------|-------|-----------|
| Depth test | `GL_LEQUAL` | Match existing passes |
| Depth write | Enabled | Edge-to-edge occlusion |
| Depth bias | `pos.z -= bias * pos.w` | Push lines towards camera past face depth |
| Alpha blend | `SRC_ALPHA, ONE_MINUS_SRC_ALPHA` | AA edge requires transparency |

Blend is enabled only during thick-line drawing and restored after.

## Pass Integration

### HighlightPass

Replace edge drawing section:

```cpp
// Before (removed):
glLineWidth(cfg.hoverEdgeVertex.lineWidth);
m_edgeShader.use();
BatchUtils::multiDrawElements(GL_LINES, batch);

// After:
ThickLineRenderer::DrawParams params{
    .positionVbo = buffers.mainVbo(),
    .indexBuffer  = buffers.ibo(),
    .mvp         = mvp,
    .viewport    = {state.viewportWidth * state.devicePixelRatio,
                    state.viewportHeight * state.devicePixelRatio},
    .lineWidth   = cfg.hoverEdgeVertex.lineWidth * state.devicePixelRatio,
    .color       = toVec4(cfg.hoverEdgeVertex.color),
    .useVertexColor = false,
    .colorMix    = 0.0F,
    .depthBias   = 0.0005F
};
thickLineRenderer.drawLines(params, edges);
```

The `m_edgeShader` and `HIGHLIGHT_EDGE_VS/FS` are **removed** from HighlightPass.

### WireframePass

Replace edge drawing section (uses `useVertexColor=true`):

```cpp
ThickLineRenderer::DrawParams params{
    .positionVbo = buffers.mainVbo(),
    .indexBuffer  = buffers.ibo(),
    .mvp         = mvp,
    .viewport    = {state.viewportWidth * state.devicePixelRatio,
                    state.viewportHeight * state.devicePixelRatio},
    .lineWidth   = 1.5F * state.devicePixelRatio,
    .color       = {},
    .useVertexColor = true,
    .colorMix    = 0.0F,
    .depthBias   = 0.0005F
};
thickLineRenderer.drawLines(params, lineRanges);
```

## Anti-Aliasing Detail

```
                     Line cross-section (screen space)

  v_edgeDist:  -1.0    -halfCore    0    +halfCore    +1.0
                │          │        │        │          │
                ▼          ▼        ▼        ▼          ▼
  alpha:        0  ████████ 1.0  ████████████  1.0 ████████  0
                   smooth    ◄──── core ────►    smooth
                   (1px)      (lineWidth px)      (1px)
```

- `halfCore = lineWidth / (lineWidth + 2.0)` — fraction of quad occupied by core
- Core region (`|v_edgeDist| < halfCore`): alpha = 1.0
- Edge region (`halfCore ≤ |v_edgeDist| ≤ 1.0`): smoothstep 1.0 → 0.0
- Total quad width = `lineWidth + 2` pixels (1px AA margin each side)

## File Change Summary

| File | Change | Estimated |
|------|--------|-----------|
| `thick_line_renderer.hpp` (NEW) | ThickLineRenderer class declaration | ~40 lines |
| `thick_line_renderer.cpp` (NEW) | Shader sources, initialize, cleanup, drawLines | ~170 lines |
| `gpu_buffer_manager.hpp` | Add `mainVbo()` and `ibo()` getters | +2 lines |
| `highlight_pass.hpp` | Remove `m_edgeShader` member | -1 line |
| `highlight_pass.cpp` | Replace edge GL_LINES with thickLineRenderer.drawLines(); remove HIGHLIGHT_EDGE_VS/FS shaders | ~-30 +10 lines |
| `wireframe_pass.hpp` | Add ThickLineRenderer reference member | +2 lines |
| `wireframe_pass.cpp` | Replace edge GL_LINES with thickLineRenderer.drawLines() | ~-10 +15 lines |
| `render_pipeline.hpp` | Add `m_thickLineRenderer` member | +2 lines |
| `render_pipeline.cpp` | Initialize/cleanup ThickLineRenderer; pass ref to passes | +6 lines |
| `color_map.hpp` | Restore lineWidth to OGL values (hover=2.5, sel=2.0) | 2 values |
| `color_map_test.cpp` | Update test assertions to match | 2 values |

**Files NOT changed**: frame_state.hpp, main.cpp, gl_viewport_renderer.cpp, batch_utils.hpp, selection_pass.cpp

## Performance

Highlight pass: typically 1-10 entities × 4-20 segments = 6-1200 vertices. Negligible.

Wireframe pass: large models may have 10K+ segments → 60K+ virtual vertices.
Each vertex does 7 `texelFetch` (cached, linear access pattern). Compared to
the `glMultiDrawElements(GL_LINES)` it replaces, overhead is:
- ~3× vertex count (6 vs 2 per segment)
- TBO fetch instead of VAO attribute read

Expected impact: < 0.5ms per frame for 50K segments on modern GPU. Acceptable.

## Testing

No new unit tests (GL rendering code). Verification:
1. Build passes (`cmake --build build --parallel 4`)
2. All 28 existing tests pass
3. Visual: hover edge shows thick + AA
4. Visual: selection edge shows thick + AA
5. Visual: wireframe edges same visual quality as before (~1.5px)
6. Performance: large model rotation smooth
