/**
 * @file render_pass_context.hpp
 * @brief Unified per-frame render context shared by render passes.
 */

#pragma once

#include "../core/gpu_buffer.hpp"
#include "render/render_data.hpp"
#include <QMatrix4x4>

namespace OpenGeoLab::Render {

// =============================================================================
// PassRenderParams — Shared rendering state passed to all render passes
// =============================================================================

/**
 * @brief Common rendering parameters shared by all render passes.
 */
struct PassRenderParams {
    QMatrix4x4 m_viewMatrix;
    QMatrix4x4 m_projMatrix;
    QVector3D m_cameraPos;
    bool m_xRayMode{false};
    RenderEntityTypeMask m_pickEntityMask{RenderEntityTypeMask::None};
};

struct GeometryPassInput {
    GpuBuffer& m_buffer;
    const std::vector<DrawRange>& m_triangleRanges;
    const std::vector<DrawRange>& m_lineRanges;
    const std::vector<DrawRange>& m_pointRanges;
    const GeometryDrawBatchCache& m_batches;
    bool hasGeometry() const {
        return !m_triangleRanges.empty() || !m_lineRanges.empty() || !m_pointRanges.empty();
    }
};

struct MeshPassInput {
    GpuBuffer& m_buffer;
    const std::vector<DrawRange>& m_triangleRanges;
    const std::vector<DrawRange>& m_lineRanges;
    const std::vector<DrawRange>& m_pointRanges;
    const MeshDrawBatchCache& m_batches;
    RenderDisplayModeMask m_displayMode{RenderDisplayModeMask::None};
    bool hasMesh() const {
        return !m_triangleRanges.empty() || !m_lineRanges.empty() || !m_pointRanges.empty();
    }
};

struct RenderPassContext {
    PassRenderParams m_params;
    GeometryPassInput m_geometry;
    MeshPassInput m_mesh;
};

} // namespace OpenGeoLab::Render
