/**
 * @file render_service.hpp
 * @brief Render service interface for managing OpenGL scene rendering
 *
 * RenderCtrlService provides the bridge between the geometry layer and the
 * OpenGL rendering system. It manages scene state, camera, and
 * coordinates render data updates when geometry changes.
 */

#pragma once

#include "geometry/geometry_document.hpp"
#include "render/render_data.hpp"
#include "util/signal.hpp"

#include <QMatrix4x4>
#include <QVector3D>
#include <functional>

namespace OpenGeoLab::Render {

/**
 * @brief Camera configuration for 3D viewport
 */
struct CameraState {
    QVector3D m_position{0.0f, 0.0f, 50.0f}; ///< Camera position in world space
    QVector3D m_target{0.0f, 0.0f, 0.0f};    ///< Look-at target point
    QVector3D m_up{0.0f, 1.0f, 0.0f};        ///< Up vector
    float m_fov{45.0f};                      ///< Field of view in degrees
    float m_nearPlane{0.1f};                 ///< Near clipping plane
    float m_farPlane{10000.0f};              ///< Far clipping plane

    /**
     * @brief Get view matrix
     * @return View transformation matrix
     */
    [[nodiscard]] QMatrix4x4 viewMatrix() const;

    /**
     * @brief Get projection matrix
     * @param aspect_ratio Viewport aspect ratio (width/height)
     * @return Perspective projection matrix
     */
    [[nodiscard]] QMatrix4x4 projectionMatrix(float aspect_ratio) const;

    /**
     * @brief Reset camera to default position
     */
    void reset();

    /**
     * @brief Fit camera to view a bounding box
     * @param bbox Target bounding box
     */
    void fitToBoundingBox(const Geometry::BoundingBox3D& bbox);
};

/**
 * @brief QML-exposed service for managing 3D scene rendering
 *
 * RenderCtrlService acts as the central coordinator for:
 * - Managing the current scene's render data
 * - Camera state and manipulation
 * - Geometry change notifications to trigger redraws
 * - Selection state management (future)
 */
class RenderCtrlService {
public:
    static RenderCtrlService& instance();

    RenderCtrlService(const RenderCtrlService&) = delete;
    RenderCtrlService& operator=(const RenderCtrlService&) = delete;
    RenderCtrlService(RenderCtrlService&&) = delete;
    RenderCtrlService& operator=(RenderCtrlService&&) = delete;

    RenderCtrlService();
    ~RenderCtrlService();

    /**
     * @brief Check if any geometry is loaded
     * @return true if render data is non-empty
     */
    [[nodiscard]] bool hasGeometry() const;

    /**
     * @brief Check if default geometry should be created
     * @return true if no geometry exists and default should be shown
     */
    [[nodiscard]] bool needsDefaultGeometry() const;

    /**
     * @brief Get current render data for the scene
     * @return Current document render data
     */
    [[nodiscard]] const DocumentRenderData& renderData() const;

    /**
     * @brief Get current camera state
     * @return Reference to camera configuration
     */
    [[nodiscard]] CameraState& camera();
    [[nodiscard]] const CameraState& camera() const;

    /**
     * @brief Replace camera state and notify listeners
     */
    void setCamera(const CameraState& camera);

    /**
     * @brief Request scene refresh
     *
     * Call this to trigger a render data update from the current document.
     */
    void refreshScene();

    /**
     * @brief Fit camera to view all geometry
     */
    void fitToScene();

    /**
     * @brief Reset camera to default view
     */
    void resetCamera();

    /**
     * @brief Create default box geometry for empty scene
     *
     * Creates a simple box to display when no model is loaded.
     */
    void createDefaultGeometry();

    /**
     * @brief Set camera to front view (looking along -Z axis)
     */
    void setFrontView();

    /**
     * @brief Set camera to top view (looking along -Y axis)
     */
    void setTopView();

    /**
     * @brief Set camera to left view (looking along +X axis)
     */
    void setLeftView();

    /**
     * @brief Set camera to right view (looking along -X axis)
     */
    void setRightView();

    void setBackView();
    void setBottomView();

    // ---------------------------------------------------------------------
    // Signals (Util::Signal based)
    // ---------------------------------------------------------------------

    [[nodiscard]] Util::ScopedConnection subscribeGeometryChanged(std::function<void()> callback);

    [[nodiscard]] Util::ScopedConnection subscribeCameraChanged(std::function<void()> callback);

    [[nodiscard]] Util::ScopedConnection subscribeSceneNeedsUpdate(std::function<void()> callback);

private:
    void subscribeToCurrentDocument();
    void updateRenderData();

    void handleDocumentGeometryChanged(const Geometry::GeometryChangeEvent& event);

private:
    DocumentRenderData m_renderData;             ///< Current scene render data
    CameraState m_camera;                        ///< Camera state
    Util::ScopedConnection m_documentConnection; ///< Connection to document changes
    bool m_hasGeometry{false};                   ///< Whether geometry is loaded

    Util::Signal<> m_geometryChanged;
    Util::Signal<> m_cameraChanged;
    Util::Signal<> m_sceneNeedsUpdate;
};

} // namespace OpenGeoLab::Render
