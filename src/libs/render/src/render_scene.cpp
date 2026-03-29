#include <opengeolab/render/render_scene.hpp>

#include <algorithm>

namespace OpenGeoLab::Render {

void RenderScene::applyChangeset(const Scene::SceneGraph::Changeset& changes,
                                 const Scene::SceneGraph& graph) {
    // Remove nodes
    for(const auto& id : changes.removed) {
        std::erase_if(m_nodes, [&](const RenderNode& n) { return n.id == id; });
    }

    // Add nodes
    for(const auto& id : changes.added) {
        const auto* sn = graph.findNode(id);
        if(sn != nullptr) {
            m_nodes.push_back(createRenderNode(*sn));
        }
    }

    // Update nodes (recreate GPU buffers for changed visual data)
    for(const auto& id : changes.updated) {
        const auto* sn = graph.findNode(id);
        if(sn == nullptr) {
            continue;
        }

        auto it = std::ranges::find_if(m_nodes, [&](const RenderNode& n) { return n.id == id; });

        if(it != m_nodes.end()) {
            *it = createRenderNode(*sn);
        } else {
            m_nodes.push_back(createRenderNode(*sn));
        }
    }
}

const std::vector<RenderNode>& RenderScene::nodes() const { return m_nodes; }

void RenderScene::clear() { m_nodes.clear(); }

RenderNode RenderScene::createRenderNode(const Scene::SceneNode& scene_node) {
    RenderNode rn;
    rn.id = scene_node.id;
    rn.modelMatrix = scene_node.transform.matrix();
    rn.style = scene_node.style;
    rn.visible = scene_node.visible;

    // Upload surfaces
    for(const auto& surf : scene_node.visual.surfaces) {
        auto gpu = GpuMesh::fromSurface(surf);
        if(gpu.isValid()) {
            rn.surfaceColors.push_back(
                {surf.defaultColor[0], surf.defaultColor[1], surf.defaultColor[2], surf.defaultColor[3]});
            rn.surfaces.push_back(std::move(gpu));
        }
    }

    // Upload edges
    for(const auto& edge : scene_node.visual.edges) {
        auto gpu = GpuMesh::fromEdges(edge);
        if(gpu.isValid()) {
            std::copy(std::begin(edge.color), std::end(edge.color), std::begin(rn.edgeColor));
            rn.edges.push_back(std::move(gpu));
        }
    }

    // Upload points (vertices)
    for(const auto& pts : scene_node.visual.points) {
        auto gpu = GpuMesh::fromPoints(pts);
        if(gpu.isValid()) {
            std::copy(std::begin(pts.color), std::end(pts.color), std::begin(rn.vertexColor));
            rn.pointSize = pts.pointSize;
            rn.points.push_back(std::move(gpu));
        }
    }

    return rn;
}

} // namespace OpenGeoLab::Render
