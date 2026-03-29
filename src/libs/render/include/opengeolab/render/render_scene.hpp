/**
 * @file render_scene.hpp
 * @brief Render-thread scene snapshot built from SceneGraph changesets.
 */

#pragma once

#include <opengeolab/core/visual_data.hpp>
#include <opengeolab/render/gpu_mesh.hpp>
#include <opengeolab/render/render_export.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <glm/mat4x4.hpp>

#include <array>
#include <string>
#include <vector>

namespace OpenGeoLab::Render {

/// A single renderable object held by the render thread.
struct RenderNode {
    std::string id;
    glm::mat4 modelMatrix{1.f};
    float defaultColor[4]{0.7f, 0.7f, 0.7f, 1.f};
    float edgeColor[4]{0.f, 0.f, 0.f, 1.f};
    float vertexColor[4]{0.204f, 0.565f, 0.871f, 1.f}; ///< #3490DE blue
    float pointSize{6.f};
    std::vector<GpuMesh> surfaces;
    std::vector<std::array<float, 4>> surfaceColors; ///< Per-surface face color
    std::vector<GpuMesh> edges;
    std::vector<GpuMesh> points;
    Core::RenderStyle style{Core::RenderStyle::SolidWithEdges};
    bool visible{true};

    RenderNode() = default;
    ~RenderNode() = default;
    RenderNode(RenderNode&&) noexcept = default;
    RenderNode& operator=(RenderNode&&) noexcept = default;
    RenderNode(const RenderNode&) = delete;
    RenderNode& operator=(const RenderNode&) = delete;
};

/// Render-thread scene snapshot updated via SceneGraph changesets.
class RenderScene {
public:
    /// Apply a changeset from SceneGraph, uploading new GPU buffers.
    void applyChangeset(const Scene::SceneGraph::Changeset& changes,
                        const Scene::SceneGraph& graph);

    [[nodiscard]] const std::vector<RenderNode>& nodes() const;

    void clear();

private:
    std::vector<RenderNode> m_nodes;

    static RenderNode createRenderNode(const Scene::SceneNode& scene_node);
};

} // namespace OpenGeoLab::Render
