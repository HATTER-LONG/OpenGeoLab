/**
 * @file render_sceneImpl.hpp
 * @brief Internal implementation of IRenderScene — owns the render passes
 *        and orchestrates per-frame rendering and GPU picking.
 */

#pragma once

#include "render/render_scene.hpp"

#include "render/pass/geometry_pass.hpp"
#include "render/pass/mesh_pass.hpp"
#include "render/pass/pick_pass.hpp"

#include <unordered_map>

namespace OpenGeoLab::Render {
/** @brief Concrete IRenderScene that composes GeometryPass, MeshPass, and PickPass. */
class RenderSceneImpl : public IRenderScene {
public:
    RenderSceneImpl();
    ~RenderSceneImpl() override;

    /** @brief Initialise geometry, mesh, and pick passes. */
    void initialize() override;
    [[nodiscard]] bool isInitialized() const override;
    /** @brief Resize the pick FBO to match the viewport.
     *  @param size New viewport size in device pixels.
     */
    void setViewportSize(const QSize& size) override;
    /** @brief Process a mouse-click pick action.
     *  @param input Pick parameters (cursor position, matrices, action type).
     */
    void processPicking(const PickingInput& input) override;
    /** @brief Execute all render passes for the current frame. */
    void render(const QVector3D& camera_pos,
                const QMatrix4x4& view_matrix,
                const QMatrix4x4& projection_matrix) override;
    /** @brief Release all GPU resources across all passes. */
    void cleanup() override;

    /**
     * @brief Process hover — render to pick FBO and update hover state.
     *
     * Call after main render with current cursor position. Uses region-based
     * picking with priority selection (Vertex > Edge > Face > Part).
     */
    void processHover(int pixel_x,
                      int pixel_y,
                      const QMatrix4x4& view,
                      const QMatrix4x4& projection) override;

private:
    /// Build uid-to-part and edge-to-wire lookups from geometry DrawRangeEx lists
    void rebuildEntityLookups();

    /// Resolve an entity uid to its parent part uid (returns 0 if not found)
    [[nodiscard]] uint64_t resolvePartUid(uint64_t entity_uid, RenderEntityType type) const;

    /// Resolve an edge uid to its parent wire uid.
    /// When face_uid is provided, prefer the wire belonging to that face
    /// (handles shared edges between faces correctly).
    /// @param edge_uid Edge entity UID.
    /// @param face_uid Optional face context from pick region (0 = no context).
    /// @return Parent wire UID, or 0 if not found.
    [[nodiscard]] uint64_t resolveWireUid(uint64_t edge_uid, uint64_t face_uid = 0) const;

    /// Get all edge UIDs belonging to a wire (empty vector if not found)
    [[nodiscard]] const std::vector<uint64_t>& resolveWireEdges(uint64_t wire_uid) const;

    GeometryPass m_geometryPass;
    MeshPass m_meshPass;
    PickPass m_pickPass;

    QSize m_viewportSize;
    bool m_initialized{false};
    bool m_pickPassInitialized{false};

    /// Entity uid → parent part uid lookup (built from DrawRangeEx data)
    std::unordered_map<uint64_t, uint64_t> m_entityToPartUid;
    /// Edge uid → parent wire uid(s) lookup (shared edges map to multiple wires)
    std::unordered_map<uint64_t, std::vector<uint64_t>> m_edgeToWireUids;
    /// Wire uid → edge uids reverse lookup for complete wire highlighting
    std::unordered_map<uint64_t, std::vector<uint64_t>> m_wireToEdgeUids;
    /// Wire uid → parent face uid lookup (each wire belongs to one face)
    std::unordered_map<uint64_t, uint64_t> m_wireToFaceUid;
};

// =============================================================================
// SceneRendererFactory implementation
// =============================================================================

/** @brief Factory registered with the component system to create RenderSceneImpl instances. */
class SceneRendererFactoryImpl : public SceneRendererFactory {
public:
    SceneRendererFactoryImpl() = default;
    ~SceneRendererFactoryImpl() = default;

    tObjectPtr create() override { return std::make_unique<RenderSceneImpl>(); }
};
} // namespace OpenGeoLab::Render
