/**
 * @file render_service.hpp
 * @brief Render service interface for managing OpenGL scene rendering
 *
 * RenderService provides the bridge between the geometry layer and the
 * OpenGL rendering system. It manages scene state, camera, and
 * coordinates render data updates when geometry changes.
 */

#pragma once

#include "geometry/geometry_document.hpp"
#include "render/render_data.hpp"

#include <QMatrix4x4>
#include <QObject>
#include <QVector3D>
#include <QtQml/qqml.h>

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
 * RenderService acts as the central coordinator for:
 * - Managing the current scene's render data
 * - Camera state and manipulation
 * - Geometry change notifications to trigger redraws
 * - Selection state management (future)
 */
class RenderService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool hasGeometry READ hasGeometry NOTIFY geometryChanged)
    Q_PROPERTY(bool needsDefaultGeometry READ needsDefaultGeometry NOTIFY geometryChanged)

public:
    explicit RenderService(QObject* parent = nullptr);
    ~RenderService() override;

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
     * @brief Request scene refresh
     *
     * Call this to trigger a render data update from the current document.
     */
    Q_INVOKABLE void refreshScene();

    /**
     * @brief Fit camera to view all geometry
     */
    Q_INVOKABLE void fitToScene();

    /**
     * @brief Reset camera to default view
     */
    Q_INVOKABLE void resetCamera();

    /**
     * @brief Create default box geometry for empty scene
     *
     * Creates a simple box to display when no model is loaded.
     */
    Q_INVOKABLE void createDefaultGeometry();

    /**
     * @brief Set camera to front view (looking along -Z axis)
     */
    Q_INVOKABLE void setFrontView();

    /**
     * @brief Set camera to top view (looking along -Y axis)
     */
    Q_INVOKABLE void setTopView();

    /**
     * @brief Set camera to left view (looking along +X axis)
     */
    Q_INVOKABLE void setLeftView();

    /**
     * @brief Set camera to right view (looking along -X axis)
     */
    Q_INVOKABLE void setRightView();

signals:
    /// Emitted when geometry data changes and viewport needs redraw
    void geometryChanged();

    /// Emitted when camera state changes
    void cameraChanged();

    /// Emitted when scene needs to be redrawn
    void sceneNeedsUpdate();

private slots:
    void onDocumentGeometryChanged(const Geometry::GeometryChangeEvent& event);

private:
    void subscribeToCurrentDocument();
    void updateRenderData();

private:
    DocumentRenderData m_renderData;             ///< Current scene render data
    CameraState m_camera;                        ///< Camera state
    Util::ScopedConnection m_documentConnection; ///< Connection to document changes
    bool m_hasGeometry{false};                   ///< Whether geometry is loaded
};

} // namespace OpenGeoLab::Render
