/**
 * @file render_scene_controller.hpp
 * @brief Scene controller that bridges geometry documents and OpenGL rendering
 *
 * RenderSceneController is the render-layer middle component:
 * - Upstream: consumed by viewport/rendering components (e.g., GLViewport)
 * - Downstream: tracks the current geometry document and produces DocumentRenderData
 * - Supports render actions to manipulate camera and trigger scene updates
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
     * @brief Build a view matrix
     * @return View transformation matrix
     */
    [[nodiscard]] QMatrix4x4 viewMatrix() const;

    /**
     * @brief Build a perspective projection matrix
     * @param aspect_ratio Viewport aspect ratio (width/height)
     * @return Perspective projection matrix
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
 * @brief Singleton controller for render scene state and data synchronization
 *
 * This component owns the current camera state and keeps DocumentRenderData
 * synchronized with the active GeometryDocument.
 *
 * Threading model:
 * - Geometry changes are bridged onto the Qt GUI thread before updating state.
 */
class RenderSceneController {
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the global RenderSceneController
     */
    static RenderSceneController& instance();

    RenderSceneController(const RenderSceneController&) = delete;
    RenderSceneController& operator=(const RenderSceneController&) = delete;
    RenderSceneController(RenderSceneController&&) = delete;
    RenderSceneController& operator=(RenderSceneController&&) = delete;

    RenderSceneController();
    ~RenderSceneController();

    /**
     * @brief Check whether any renderable geometry exists
     * @return true if the current render data is non-empty
     */
    [[nodiscard]] bool hasGeometry() const;

    /**
     * @brief Get the current render data snapshot
     * @return Reference to the current DocumentRenderData
     */
    [[nodiscard]] const DocumentRenderData& renderData() const;

    /**
     * @brief Get or set current camera state
     */
    [[nodiscard]] CameraState& camera();
    [[nodiscard]] const CameraState& camera() const;

    /**
     * @brief Replace camera state and notify listeners
     * @param camera New camera configuration
     */
    void setCamera(const CameraState& camera);

    /**
     * @brief Refresh render data from the current geometry document
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
     * @brief Set camera to a front view (view direction along -Z)
     */
    void setFrontView();

    /**
     * @brief Set camera to a back view (view direction along +Z)
     */
    void setBackView();

    /**
     * @brief Set camera to a top view (view direction along -Y)
     */
    void setTopView();

    /**
     * @brief Set camera to a bottom view (view direction along +Y)
     */
    void setBottomView();

    /**
     * @brief Set camera to a left view (view direction along +X)
     */
    void setLeftView();

    /**
     * @brief Set camera to a right view (view direction along -X)
     */
    void setRightView();

    // ---------------------------------------------------------------------
    // Signals (Util::Signal based)
    // ---------------------------------------------------------------------

    /**
     * @brief Subscribe to render data changes
     * @param callback Callback executed when render data updates
     * @return Scoped connection that disconnects on destruction
     */
    [[nodiscard]] Util::ScopedConnection subscribeGeometryChanged(std::function<void()> callback);

    /**
     * @brief Subscribe to camera changes
     * @param callback Callback executed when camera updates
     * @return Scoped connection that disconnects on destruction
     */
    [[nodiscard]] Util::ScopedConnection subscribeCameraChanged(std::function<void()> callback);

    /**
     * @brief Subscribe to scene update requests
     *
     * This signal indicates a redraw should happen (e.g., camera changed).
     */
    [[nodiscard]] Util::ScopedConnection subscribeSceneNeedsUpdate(std::function<void()> callback);

private:
    void subscribeToCurrentDocument();
    void subscribeToDocument(const Geometry::GeometryDocumentPtr& document);
    void updateRenderData();

    void handleDocumentGeometryChanged(const Geometry::GeometryChangeEvent& event);

private:
    DocumentRenderData m_renderData;                 ///< Current render data snapshot
    CameraState m_camera;                            ///< Current camera state
    Geometry::GeometryDocumentPtr m_currentDocument; ///< Currently subscribed document
    Util::ScopedConnection m_documentConnection;     ///< Connection to document changes
    bool m_hasGeometry{false};                       ///< Whether geometry is loaded

    Util::Signal<> m_geometryChanged;
    Util::Signal<> m_cameraChanged;
    Util::Signal<> m_sceneNeedsUpdate;
};

} // namespace OpenGeoLab::Render
