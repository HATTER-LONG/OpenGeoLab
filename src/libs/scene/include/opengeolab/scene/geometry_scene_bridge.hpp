/**
 * @file geometry_scene_bridge.hpp
 * @brief GeometrySceneBridge — automatic ShapeStore-to-SceneGraph synchronizer
 *
 * Listens to ShapeStore signals and creates/removes SceneNodes with
 * RenderMeshData, IRenderComponent, and IPickComponent automatically.
 * Also maintains TopologyIndex for pick-mode escalation.
 */

#pragma once

#include <opengeolab/scene/render_mesh_data.hpp>
#include <opengeolab/scene/scene_export.hpp>
#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/topology_index.hpp>

#include <kangaroo/util/signal.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Geometry {
class ShapeStore;
struct ShapeEntry;
} // namespace OpenGeoLab::Geometry

namespace OpenGeoLab::Scene {

/**
 * @brief Synchronizes ShapeStore changes into SceneGraph nodes.
 *
 * On construction, connects to ShapeStore signals. On destruction,
 * disconnects. For each added shape, creates a SceneNode with
 * concrete IRenderComponent and IPickComponent, and builds topology index.
 */
class OPENGEOLAB_SCENE_EXPORT GeometrySceneBridge final {
public:
    /**
     * @brief Construct and connect to ShapeStore signals.
     * @param scene SceneGraph to populate
     * @param store ShapeStore to observe
     * @param topoIndex TopologyIndex to maintain
     */
    GeometrySceneBridge(SceneGraph& scene, Geometry::ShapeStore& store, TopologyIndex& topoIndex);

    ~GeometrySceneBridge();

    GeometrySceneBridge(const GeometrySceneBridge&) = delete;
    GeometrySceneBridge& operator=(const GeometrySceneBridge&) = delete;
    GeometrySceneBridge(GeometrySceneBridge&&) = delete;
    GeometrySceneBridge& operator=(GeometrySceneBridge&&) = delete;

    /**
     * @brief Build RenderMeshData from a ShapeEntry's VisualData.
     *
     * Converts SurfaceMesh → triangle DrawRanges + RenderVertex/PickIdEntry,
     * EdgeMesh → line DrawRanges, PointSet → point DrawRanges.
     */
    [[nodiscard]] static RenderMeshData buildRenderData(uint32_t shapeId,
                                                        const Geometry::ShapeEntry& entry);

private:
    void onShapeAdded(uint32_t shapeId, const Geometry::ShapeEntry& entry);
    void onShapeRemoved(uint32_t shapeId);
    void onShapeUpdated(uint32_t shapeId, const Geometry::ShapeEntry& entry);

    SceneGraph& m_scene;
    Geometry::ShapeStore& m_store;
    TopologyIndex& m_topoIndex;

    /** @brief Maps shapeId → NodeId for quick lookup on remove/update. */
    std::unordered_map<uint32_t, NodeId> m_shapeToNode;

    /** @brief Signal connections for cleanup on destruction. */
    std::vector<Kangaroo::Util::ScopedConnection> m_connections;
};

} // namespace OpenGeoLab::Scene
