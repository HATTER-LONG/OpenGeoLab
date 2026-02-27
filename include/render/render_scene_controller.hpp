/**
 * @file render_scene_controller.hpp
 * @brief Central controller managing camera state, render data, and scene lifecycle.
 */

#pragma once

#include "geometry/geometry_document.hpp"
#include "geometry/geometry_types.hpp"
#include "render/render_data.hpp"

#include <QMatrix4x4>
#include <QVector3D>
#include <kangaroo/util/noncopyable.hpp>

#include <atomic>
#include <mutex>

namespace OpenGeoLab::Render {
/**
 * @brief Camera configuration for a 3D viewport
 */
struct CameraState {
    QVector3D m_position{0.0f, 0.0f, 50.0f}; ///< Camera position in world space
    QVector3D m_target{0.0f, 0.0f, 0.0f};    ///< Look-at target point
    QVector3D m_up{0.0f, 1.0f, 0.0f};        ///< Up vector
    float m_fov{45.0f};                      ///< Field of view in degrees
    float m_nearPlane{0.1f};                 ///< Near clipping plane
    float m_farPlane{10000.0f};              ///< Far clipping plane

    /**
     * @brief Update clipping planes based on the current camera distance
     * @param distance Distance between camera position and target
     */
    void updateClipping(float distance);

    /**
     * @brief Build a view matrix
     * @return View transformation matrix
     */
    [[nodiscard]] QMatrix4x4 viewMatrix() const;

    /**
     * @brief Build an orthographic projection matrix
     * @param aspect_ratio Viewport aspect ratio (width/height)
     * @return Projection matrix
     */
    [[nodiscard]] QMatrix4x4 projectionMatrix(float aspect_ratio) const;

    /**
     * @brief Reset camera to default configuration
     */
    void reset();

    /**
     * @brief Fit camera to view a bounding box
     * @param bbox Target bounding box
     */
    void fitToBoundingBox(const Geometry::BoundingBox3D& bbox);
};

/**
 * @brief Describes what changed in the scene, so listeners can react accordingly.
 */
enum class SceneUpdateType : uint8_t {
    GeometryChanged = 0,
    MeshChanged = 1,
    CameraChanged = 2,
};

/**
 * @brief Singleton coordinating camera state, render data, and document subscriptions.
 *
 * Bridges geometry/mesh documents with the render layer: listens for document
 * changes, rebuilds render data, and notifies the viewport to schedule redraws.
 * Also provides camera presets (front/back/top/...) and scene-fit operations.
 */
class RenderSceneController : public Kangaroo::Util::NonCopyMoveable {
protected:
    RenderSceneController();

public:
    static RenderSceneController& instance();
    virtual ~RenderSceneController();

    virtual CameraState& cameraState() { return m_cameraState; }

    /**
     * @brief Replace the current camera state.
     * @param camera New camera configuration.
     * @param notify If true, emit SceneUpdateType::CameraChanged to trigger a redraw.
     */
    void setCamera(const CameraState& camera, bool notify = true);

    /** @brief Rebuild all render data from the current documents and schedule a redraw. */
    void refreshScene(bool notify = true);

    /** @brief Fit the camera to the bounding box of all visible geometry. */
    void fitToScene(bool notify = true);

    /** @brief Reset the camera to its default position and orientation. */
    void resetCamera(bool notify = true);

    /** @brief Set camera to front orthographic view. */
    void setFrontView(bool notify = true);

    /** @brief Set camera to back orthographic view. */
    void setBackView(bool notify = true);

    /** @brief Set camera to top orthographic view. */
    void setTopView(bool notify = true);

    /** @brief Set camera to bottom orthographic view. */
    void setBottomView(bool notify = true);

    /** @brief Set camera to left orthographic view. */
    void setLeftView(bool notify = true);

    /** @brief Set camera to right orthographic view. */
    void setRightView(bool notify = true);

    /**
     * @brief Toggle X-ray mode (semi-transparent faces to see edges through surfaces)
     */
    void toggleXRayMode(bool notify = true);

    /** @brief Check whether X-ray mode is currently enabled. */
    [[nodiscard]] bool isXRayMode() const noexcept;

    /**
     * @brief Cycle mesh display mode: Wireframe -> Surface+Points -> Surface+Points+Wireframe ->
     * ...
     */
    void cycleMeshDisplayMode(bool notify = true);

    /** @brief Current mesh display mode bitmask. */
    [[nodiscard]] RenderDisplayModeMask meshDisplayMode() const noexcept;

    /**
     * @brief Read-only access to the current render data snapshot.
     *
     * Thread-safety contract: this reference is read by the render thread during
     * render() and processHover/processPicking. The GUI thread rebuilds the data
     * in updateGeometryRenderData/updateMeshRenderData. Synchronization relies on
     * Qt Scene Graph's synchronize() barrier — the GUI thread is blocked while
     * synchronize() runs, and render data is not modified during render(). If the
     * data-update frequency increases or a non-Qt renderer is introduced, consider
     * adding a shared_mutex or double-buffering.
     */
    [[nodiscard]] const RenderData& renderData() const;

    /** @brief Current geometry document (may be null if no document is loaded). */
    [[nodiscard]] Geometry::GeometryDocumentPtr currentGeometryDocument() const;

    /** @brief Show or hide a part's geometry (CAD shape) rendering. */
    void setPartGeometryVisible(Geometry::EntityUID part_uid, bool visible);

    /** @brief Show or hide a part's mesh rendering. */
    void setPartMeshVisible(Geometry::EntityUID part_uid, bool visible);
    [[nodiscard]] bool isPartGeometryVisible(Geometry::EntityUID part_uid) const;
    [[nodiscard]] bool isPartMeshVisible(Geometry::EntityUID part_uid) const;

    /**
     * @brief Subscribe to scene-update notifications.
     * @param callback Invoked with the SceneUpdateType whenever
     *        geometry, mesh, or camera data changes.
     * @return ScopedConnection that automatically unsubscribes on destruction.
     */
    [[nodiscard]] Util::ScopedConnection
    subscribeToSceneNeedsUpdate(std::function<void(SceneUpdateType)> callback) {
        return m_sceneNeedsUpdate.connect(callback);
    }

private:
    void subscribeToGeometryDocument();

    void subscribeToMeshDocument();

    void handleDocumentGeometryChanged(const Geometry::GeometryChangeEvent& event);

    void handleDocumentMeshChanged();

    void updateGeometryRenderData();

    void updateMeshRenderData();

private:
    CameraState m_cameraState; ///< Current camera configuration

    Util::ScopedConnection m_geometryDocumentConnection; ///< Geometry-change subscription
    Util::ScopedConnection m_meshDocumentConnection;     ///< Mesh-change subscription

    Util::Signal<SceneUpdateType> m_sceneNeedsUpdate; ///< Notifies listeners to redraw

    RenderData m_renderData; ///< Snapshot of all renderable data (geometry + mesh)

    /// Per-part visibility toggles for geometry and mesh layers
    struct PartVisibility {
        bool m_geometryVisible{true}; ///< CAD shape visible
        bool m_meshVisible{true};     ///< FEM mesh visible
    };
    mutable std::mutex m_visibilityMutex; ///< Guards m_partVisibility
    std::unordered_map<Geometry::EntityUID, PartVisibility> m_partVisibility;

    std::atomic<uint8_t> m_meshDisplayMode{
        static_cast<uint8_t>(RenderDisplayModeMask::Wireframe |
                             RenderDisplayModeMask::Points)}; ///< Mesh display mode bitmask

    std::atomic<bool> m_xRayMode{false}; ///< X-ray transparency toggle
};
} // namespace OpenGeoLab::Render