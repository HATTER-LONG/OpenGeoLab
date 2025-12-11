/**
 * @file scene_controller.hpp
 * @brief Scene controller for managing 3D scene state and interactions
 *
 * Provides a high-level interface for controlling 3D scene rendering:
 * - Model transformation (rotation, translation, scale)
 * - View manipulation (fit to bounds, standard views)
 * - Geometry management
 *
 * This component separates scene control logic from the OpenGL rendering details.
 */

#pragma once

#include <geometry/geometry.hpp>
#include <render/opengl_renderer.hpp>

#include <QObject>
#include <QVector3D>
#include <memory>

namespace OpenGeoLab {
namespace Rendering {

/**
 * @brief Scene controller for managing 3D scene interactions
 *
 * This class provides a clean interface between UI controls and the rendering system.
 * It handles:
 * - Model transformations (rotation via quaternion for smooth arcball rotation)
 * - View controls (standard views, fit to bounds)
 * - Geometry loading and management
 */
class SceneController : public QObject {
    Q_OBJECT

public:
    explicit SceneController(QObject* parent = nullptr);
    ~SceneController() override = default;

    /**
     * @brief Set the renderer to control
     * @param renderer Pointer to the OpenGL renderer
     */
    void setRenderer(OpenGLRenderer* renderer);

    /**
     * @brief Get the controlled renderer
     * @return Pointer to renderer, or nullptr if not set
     */
    OpenGLRenderer* renderer() const { return m_renderer; }

    // ========================================================================
    // Geometry Management
    // ========================================================================

    /**
     * @brief Set geometry data for rendering
     * @param geometry_data Shared pointer to geometry data
     *
     * Automatically calculates bounds and sets up model center for rotation.
     */
    void setGeometryData(std::shared_ptr<Geometry::GeometryData> geometry_data);

    /**
     * @brief Check if geometry bounds are available
     * @return true if bounds have been calculated
     */
    bool hasBounds() const { return m_hasBounds; }

    /**
     * @brief Get geometry bounding box minimum point
     * @return Minimum corner of bounding box
     */
    QVector3D boundsMin() const { return m_boundsMin; }

    /**
     * @brief Get geometry bounding box maximum point
     * @return Maximum corner of bounding box
     */
    QVector3D boundsMax() const { return m_boundsMax; }

    // ========================================================================
    // Model Transformation
    // ========================================================================

    /**
     * @brief Rotate model by delta angles (arcball-style)
     * @param delta_yaw Horizontal rotation in degrees
     * @param delta_pitch Vertical rotation in degrees
     */
    void rotateModel(float delta_yaw, float delta_pitch);

    /**
     * @brief Reset model rotation to identity
     */
    void resetModelRotation();

    // ========================================================================
    // View Controls
    // ========================================================================

    /**
     * @brief Zoom view by factor
     * @param factor Zoom multiplier (> 1 zooms in, < 1 zooms out)
     */
    void zoom(float factor);

    /**
     * @brief Pan view in screen space
     * @param delta_x Horizontal pan amount
     * @param delta_y Vertical pan amount
     */
    void pan(float delta_x, float delta_y);

    /**
     * @brief Fit view to show entire geometry
     * @param margin Extra margin around geometry (default 1.5)
     */
    void fitToView(float margin = 1.5f);

    /**
     * @brief Reset view to default camera position
     */
    void resetView();

    // ========================================================================
    // Standard Views
    // ========================================================================

    /** @brief Set view to front (looking at -Z) */
    void setViewFront();

    /** @brief Set view to back (looking at +Z) */
    void setViewBack();

    /** @brief Set view to top (looking at -Y) */
    void setViewTop();

    /** @brief Set view to bottom (looking at +Y) */
    void setViewBottom();

    /** @brief Set view to left (looking at -X) */
    void setViewLeft();

    /** @brief Set view to right (looking at +X) */
    void setViewRight();

    /** @brief Set view to isometric (45° yaw, ~35° pitch) */
    void setViewIsometric();

signals:
    /**
     * @brief Emitted when geometry data changes
     */
    void geometryChanged();

    /**
     * @brief Emitted when view/model transformation changes
     */
    void viewChanged();

private:
    OpenGLRenderer* m_renderer = nullptr;

    // Cached bounds
    bool m_hasBounds = false;
    QVector3D m_boundsMin;
    QVector3D m_boundsMax;
};

} // namespace Rendering
} // namespace OpenGeoLab
