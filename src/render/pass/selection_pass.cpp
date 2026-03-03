/**
 * @file selection_pass.cpp
 * @brief SelectionPass implementation — GPU picking with priority-based analysis
 */

#include "selection_pass.hpp"

#include "util/color_map.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>

#include <algorithm>
#include <cfloat>
#include <climits>

namespace OpenGeoLab::Render {
namespace {

const char* pick_vertex_shader = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 3) in uvec2 a_pickId;
uniform mat4 u_viewMatrix;
uniform mat4 u_projMatrix;
uniform float u_pointSize;
flat out uvec2 v_pickId;
void main() {
    v_pickId = a_pickId;
    gl_PointSize = u_pointSize;
    gl_Position = u_projMatrix * u_viewMatrix * vec4(a_position, 1.0);
}
)";

const char* pick_fragment_shader = R"(
#version 330 core
flat in uvec2 v_pickId;
layout(location = 0) out uvec2 fragPickId;
void main() {
    fragPickId = v_pickId;
}
)";

} // namespace

void SelectionPass::initialize(int width, int height) {
    if(m_initialized) {
        return;
    }
    if(!m_pickShader.compile(pick_vertex_shader, pick_fragment_shader)) {
        LOG_ERROR("SelectionPass: Failed to compile pick shader");
        return;
    }
    m_pickFBO.initialize(width, height);
    if(!m_pickFBO.isInitialized()) {
        LOG_ERROR("SelectionPass: Failed to initialize PickFBO");
        return;
    }
    m_initialized = true;
    LOG_DEBUG("SelectionPass: Initialized ({}x{})", width, height);
}

void SelectionPass::resize(int width, int height) { m_pickFBO.resize(width, height); }

void SelectionPass::cleanup() {
    m_pickFBO.cleanup();
    m_initialized = false;
    LOG_DEBUG("SelectionPass: Cleaned up");
}

PickAnalysisResult SelectionPass::pick(QOpenGLFunctions* f,
                                       GpuBuffer& gpu_buffer,
                                       const QMatrix4x4& view,
                                       const QMatrix4x4& projection,
                                       int pixel_x,
                                       int pixel_y,
                                       RenderEntityTypeMask pick_mask,
                                       const std::vector<DrawRange>& triangle_ranges,
                                       const std::vector<DrawRange>& line_ranges,
                                       const std::vector<DrawRange>& point_ranges) {
    if(!m_initialized) {
        return {};
    }

    // 1. Bind the PickFBO and set up GL state
    m_pickFBO.bind();
    f->glEnable(GL_DEPTH_TEST);
    f->glDepthFunc(GL_LESS);
    f->glDisable(GL_BLEND);
    f->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    f->glDepthMask(GL_TRUE);

    gpu_buffer.bindForDraw();
    m_pickShader.bind();
    m_pickShader.setUniformMatrix4("u_viewMatrix", view);
    m_pickShader.setUniformMatrix4("u_projMatrix", projection);

    // 2. Render only the topologies matching the pick mask
    if(shouldRenderTriangles(pick_mask) && !triangle_ranges.empty()) {
        uint32_t total_count = 0;
        for(const auto& r : triangle_ranges) {
            total_count += r.m_indexCount;
        }
        if(total_count > 0) {
            const uint32_t first_offset = triangle_ranges.front().m_indexOffset;
            f->glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(total_count), GL_UNSIGNED_INT,
                              reinterpret_cast<const void*>(static_cast<uintptr_t>(first_offset) *
                                                            sizeof(uint32_t)));
        }
    }

    if(shouldRenderLines(pick_mask) && !line_ranges.empty()) {
        uint32_t total_count = 0;
        for(const auto& r : line_ranges) {
            total_count += r.m_indexCount;
        }
        if(total_count > 0) {
            const uint32_t first_offset = line_ranges.front().m_indexOffset;
            // Use wider lines for easier picking
            f->glLineWidth(Util::RenderStyle::EDGE_LINE_WIDTH_HOVER);
            f->glDrawElements(GL_LINES, static_cast<GLsizei>(total_count), GL_UNSIGNED_INT,
                              reinterpret_cast<const void*>(static_cast<uintptr_t>(first_offset) *
                                                            sizeof(uint32_t)));
            f->glLineWidth(1.0f);
        }
    }

    if(shouldRenderPoints(pick_mask) && !point_ranges.empty()) {
        uint32_t total_count = 0;
        for(const auto& r : point_ranges) {
            total_count += r.m_vertexCount;
        }
        if(total_count > 0) {
            const uint32_t first_vertex = point_ranges.front().m_vertexOffset;
            f->glEnable(GL_PROGRAM_POINT_SIZE);
            // Use larger points for easier picking
            m_pickShader.setUniformFloat("u_pointSize", Util::RenderStyle::VERTEX_POINT_SIZE *
                                                            Util::RenderStyle::VERTEX_SCALE_HOVER);
            f->glDrawArrays(GL_POINTS, static_cast<GLint>(first_vertex),
                            static_cast<GLsizei>(total_count));
        }
    }

    m_pickShader.release();
    gpu_buffer.unbind();

    // 3. Read back a region around the cursor
    const int half = PICK_REGION_SIZE / 2;
    int rx = std::max(0, pixel_x - half);
    int ry = std::max(0, pixel_y - half);
    int rw = std::min(PICK_REGION_SIZE, m_pickFBO.width() - rx);
    int rh = std::min(PICK_REGION_SIZE, m_pickFBO.height() - ry);

    auto pixels = m_pickFBO.readRegion(rx, ry, rw, rh);

    m_pickFBO.unbind();

    if(pixels.empty()) {
        return {};
    }

    // 4. Analyze with priority
    return analyzeRegion(pixels, rw, rh, pick_mask);
}

PickAnalysisResult SelectionPass::analyzeRegion(const std::vector<PickPixel>& pixels,
                                                int region_w,
                                                int region_h,
                                                RenderEntityTypeMask pick_mask) const {
    PickAnalysisResult best;
    int best_priority = INT_MAX;
    float best_distance_sq = FLT_MAX;
    const float center_x = static_cast<float>(region_w) / 2.0f;
    const float center_y = static_cast<float>(region_h) / 2.0f;

    for(int y = 0; y < region_h; ++y) {
        for(int x = 0; x < region_w; ++x) {
            const auto& px = pixels[static_cast<size_t>(y * region_w + x)];
            if(px.isBackground()) {
                continue;
            }

            const uint64_t raw_id = px.pickId();
            const auto type = PickId::decodeType(raw_id);
            const auto uid = PickId::decodeUID(raw_id);

            // Skip types not in current pick mask
            if((toMask(type) & pick_mask) == RenderEntityTypeMask::None) {
                continue;
            }

            const int priority = typePriority(type);
            const float dx = static_cast<float>(x) - center_x;
            const float dy = static_cast<float>(y) - center_y;
            const float dist_sq = dx * dx + dy * dy;

            // Prefer higher priority (lower number), then closer to center
            if(priority < best_priority ||
               (priority == best_priority && dist_sq < best_distance_sq)) {
                best.m_uid = uid;
                best.m_type = type;
                best.m_rawPickId = raw_id;
                best_priority = priority;
                best_distance_sq = dist_sq;
            }
        }
    }

    return best;
}

bool SelectionPass::shouldRenderTriangles(RenderEntityTypeMask mask) {
    constexpr auto face_like = RenderEntityTypeMask::Face | RenderEntityTypeMask::Shell |
                               RenderEntityTypeMask::Solid | RenderEntityTypeMask::CompSolid |
                               RenderEntityTypeMask::Compound | RenderEntityTypeMask::Part |
                               RENDER_MESH_ELEMENTS;
    return (mask & face_like) != RenderEntityTypeMask::None;
}

bool SelectionPass::shouldRenderLines(RenderEntityTypeMask mask) {
    constexpr auto line_like =
        RenderEntityTypeMask::Edge | RenderEntityTypeMask::Wire | RenderEntityTypeMask::MeshLine;
    return (mask & line_like) != RenderEntityTypeMask::None;
}

bool SelectionPass::shouldRenderPoints(RenderEntityTypeMask mask) {
    constexpr auto point_like = RenderEntityTypeMask::Vertex | RenderEntityTypeMask::MeshNode;
    return (mask & point_like) != RenderEntityTypeMask::None;
}

int SelectionPass::typePriority(RenderEntityType type) {
    switch(type) {
    case RenderEntityType::Vertex:
        return 0;
    case RenderEntityType::MeshNode:
        return 1;
    case RenderEntityType::Edge:
        return 2;
    case RenderEntityType::MeshLine:
        return 3;
    case RenderEntityType::Face:
        return 4;
    case RenderEntityType::MeshTriangle:
    case RenderEntityType::MeshQuad4:
        return 5;
    case RenderEntityType::MeshTetra4:
    case RenderEntityType::MeshHexa8:
    case RenderEntityType::MeshPrism6:
    case RenderEntityType::MeshPyramid5:
        return 6;
    case RenderEntityType::Wire:
        return 7;
    case RenderEntityType::Shell:
    case RenderEntityType::Solid:
    case RenderEntityType::CompSolid:
    case RenderEntityType::Compound:
        return 8;
    case RenderEntityType::Part:
        return 9;
    default:
        return 99;
    }
}

} // namespace OpenGeoLab::Render
