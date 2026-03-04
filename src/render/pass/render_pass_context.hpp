/**
 * @file render_pass_context.hpp
 * @brief Unified per-frame render context shared by render passes.
 */

#pragma once

#include "render/render_data.hpp"

namespace OpenGeoLab::Render {

class GpuBuffer;

struct GeometryPassInput {
    GpuBuffer* m_buffer{nullptr};
    const std::vector<DrawRangeEx>* m_triangleRanges{nullptr};
    const std::vector<DrawRangeEx>* m_lineRanges{nullptr};
    const std::vector<DrawRangeEx>* m_pointRanges{nullptr};
};

struct MeshPassInput {
    GpuBuffer* m_buffer{nullptr};
    const std::vector<DrawRangeEx>* m_triangleRanges{nullptr};
    const std::vector<DrawRangeEx>* m_lineRanges{nullptr};
    const std::vector<DrawRangeEx>* m_pointRanges{nullptr};
    RenderDisplayModeMask m_displayMode{RenderDisplayModeMask::None};
};

struct RenderPassContext {
    PassRenderParams m_params;
    GeometryPassInput m_geometry;
    MeshPassInput m_mesh;
};

} // namespace OpenGeoLab::Render
