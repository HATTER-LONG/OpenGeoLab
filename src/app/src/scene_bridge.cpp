/// @file scene_bridge.cpp
/// @brief SceneBridge implementation — syncs geometry/mesh to SceneGraph.
#include "scene_bridge.h"

#include <opengeolab/command/command_dispatcher.hpp>
#include <opengeolab/core/logger.hpp>
#include <opengeolab/core/shape_color_palette.hpp>
#include <opengeolab/geometry/geometry_module.hpp>
#include <opengeolab/mesh/mesh_module.hpp>
#include <opengeolab/mesh/mesh_visual_builder.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_set>

namespace OpenGeoLab::App {

namespace {

/// Edge color — golden yellow matching OGL reference (#FFD460).
constexpr float kEdgeColor[4] = {1.000f, 0.831f, 0.376f, 1.f};

/// Vertex color — blue matching OGL reference (#3490DE).
constexpr float kVertexColor[4] = {0.204f, 0.565f, 0.871f, 1.f};

/// Vertex point size for display.
constexpr float kVertexPointSize = 6.f;

/// Assign all faces of a shape the same colour from the palette,
/// indexed by shape id.  Edges and vertices use unified colours.
void applyColorMap(Core::VisualData& visual, uint32_t shape_id) {
    const auto& c = Core::kShapeColorPalette[shape_id % Core::kShapeColorPaletteSize];
    for(auto& surf : visual.surfaces) {
        std::copy(c.begin(), c.end(), std::begin(surf.defaultColor));
    }
    for(auto& edge : visual.edges) {
        std::copy(std::begin(kEdgeColor), std::end(kEdgeColor), std::begin(edge.color));
    }
    for(auto& pts : visual.points) {
        std::copy(std::begin(kVertexColor), std::end(kVertexColor), std::begin(pts.color));
        pts.pointSize = kVertexPointSize;
    }
}

} // namespace

SceneBridge::SceneBridge(Command::CommandDispatcher& dispatcher,
                         Scene::SceneGraph& scene_graph,
                         QObject* parent)
    : QObject(parent), m_dispatcher(dispatcher), m_sceneGraph(scene_graph) {}

void SceneBridge::onGeometryDataChanged() {
    syncGeometryToScene();
    Q_EMIT sceneUpdated();
}

void SceneBridge::onMeshDataChanged() {
    syncMeshToScene();
    Q_EMIT sceneUpdated();
}

void SceneBridge::setGeoVisible(int shapeId, bool visible) {
    auto id = static_cast<uint32_t>(shapeId);
    if(m_shapeNodeMap.count(id) != 0) {
        m_sceneGraph.setNodeVisibility(m_shapeNodeMap[id], visible);
        Q_EMIT sceneUpdated();
    }
}

void SceneBridge::setMeshVisible(int meshId, bool visible) {
    auto id = static_cast<uint32_t>(meshId);
    if(m_meshNodeMap.count(id) != 0) {
        m_sceneGraph.setNodeVisibility(m_meshNodeMap[id], visible);
        Q_EMIT sceneUpdated();
    }
}

void SceneBridge::syncGeometryToScene() {
    auto base = m_dispatcher.findModule("geometry");
    if(!base) {
        return;
    }

    auto geo_module = std::dynamic_pointer_cast<Geometry::GeometryModule>(base);
    if(!geo_module) {
        return;
    }

    auto& store = geo_module->shapeStore();
    auto current_ids = store.allShapeIds();
    std::unordered_set<uint32_t> current_set(current_ids.begin(), current_ids.end());

    // Remove nodes for shapes that no longer exist
    for(auto it = m_shapeNodeMap.begin(); it != m_shapeNodeMap.end();) {
        if(current_set.count(it->first) == 0) {
            m_sceneGraph.removeNode(it->second);
            it = m_shapeNodeMap.erase(it);
        } else {
            ++it;
        }
    }

    // Add or update nodes
    for(uint32_t shape_id : current_ids) {
        const auto* entry = store.find(shape_id);
        if(entry == nullptr) {
            continue;
        }

        // Ensure tessellation exists
        if(!entry->visualData) {
            store.tessellate(shape_id);
            entry = store.find(shape_id);
            if(entry == nullptr || !entry->visualData) {
                continue;
            }
        }

        std::string node_id = "shape_" + std::to_string(shape_id);

        if(m_shapeNodeMap.count(shape_id) == 0) {
            // New node
            Scene::SceneNode node;
            node.id = node_id;
            node.entity = {Core::EntityType::GeoSolid, shape_id};
            node.visual = *entry->visualData;
            applyColorMap(node.visual, shape_id);
            node.style = Core::RenderStyle::SolidWithEdges;

            // Compute bounds from surface data
            for(const auto& surf : node.visual.surfaces) {
                if(!surf.positions.empty()) {
                    node.bounds.expand(Scene::BoundingBox::fromPositions(
                        surf.positions.data(), surf.positions.size() / 3, sizeof(float) * 3));
                }
            }

            m_sceneGraph.addNode(std::move(node));
            m_shapeNodeMap[shape_id] = node_id;
        } else {
            // Update existing — re-apply palette colours before pushing to scene.
            auto visual = *entry->visualData;
            applyColorMap(visual, shape_id);
            m_sceneGraph.updateVisual(node_id, std::move(visual));
        }
    }
}

void SceneBridge::syncMeshToScene(){
    auto base = m_dispatcher.findModule("mesh");
    if(!base) {
        return;
    }

    auto mesh_module = std::dynamic_pointer_cast<Mesh::MeshModule>(base);
    if(!mesh_module) {
        return;
    }

    auto& store = mesh_module->meshStore();
    auto current_ids = store.allMeshIds();
    std::unordered_set<uint32_t> current_set(current_ids.begin(), current_ids.end());

    // Remove nodes for meshes that no longer exist
    for(auto it = m_meshNodeMap.begin(); it != m_meshNodeMap.end();) {
        if(current_set.count(it->first) == 0) {
            m_sceneGraph.removeNode(it->second);
            it = m_meshNodeMap.erase(it);
        } else {
            ++it;
        }
    }

    // Add or update nodes
    for(uint32_t mesh_id : current_ids) {
        auto entry = store.find(mesh_id);
        if(!entry) {
            continue;
        }

        // Build visual data if not cached
        if(!entry->visualData) {
            auto mutable_entry = store.findMutable(mesh_id);
            if(mutable_entry) {
                mutable_entry->visualData = std::make_shared<Core::VisualData>(
                    Mesh::MeshVisualBuilder::buildVisualData(*mutable_entry));
            }
            entry = store.find(mesh_id);
            if(!entry || !entry->visualData) {
                continue;
            }
        }

        std::string node_id = "mesh_" + std::to_string(mesh_id);

        if(m_meshNodeMap.count(mesh_id) == 0) {
            Scene::SceneNode node;
            node.id = node_id;
            node.entity = {Core::EntityType::MeshElement, mesh_id};
            node.visual = *entry->visualData;
            applyColorMap(node.visual, mesh_id);
            node.style = Core::RenderStyle::SolidWithEdges;

            for(const auto& surf : node.visual.surfaces) {
                if(!surf.positions.empty()) {
                    node.bounds.expand(Scene::BoundingBox::fromPositions(
                        surf.positions.data(), surf.positions.size() / 3, sizeof(float) * 3));
                }
            }

            m_sceneGraph.addNode(std::move(node));
            m_meshNodeMap[mesh_id] = node_id;
        } else {
            auto visual = *entry->visualData;
            applyColorMap(visual, mesh_id);
            m_sceneGraph.updateVisual(node_id, std::move(visual));
        }
    }
}

} // namespace OpenGeoLab::App
