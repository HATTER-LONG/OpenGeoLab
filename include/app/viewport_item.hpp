/**
 * @file viewport_item.hpp
 * @brief QML viewport item for 3D geometry rendering
 *
 * Provides a QQuickFramebufferObject-based viewport that integrates
 * the OpenGL renderer with the Qt Quick scene graph.
 */

#pragma once

#include "geometry/geometry_document.hpp"
#include "render/gl_renderer.hpp"
#include "render/tessellator.hpp"

#include <QQuickFramebufferObject>
#include <QtQml/qqml.h>

#include <memory>

namespace OpenGeoLab::App {

class ViewportItem;

/**
 * @brief Renderer for ViewportItem that runs in the scene graph thread
 *
 * Uses document version tracking to detect changes and trigger scene rebuilds,
 * since the observer pattern cannot be safely used across Qt's render thread boundary.
 */
class ViewportRenderer : public QQuickFramebufferObject::Renderer {
public:
    explicit ViewportRenderer(ViewportItem* item);
    ~ViewportRenderer() override;

    /**
     * @brief Create the framebuffer object
     */
    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override;

    /**
     * @brief Render the scene
     */
    void render() override;

    /**
     * @brief Synchronize state from the main thread
     */
    void synchronize(QQuickFramebufferObject* item) override;

private:
    void rebuildScene();

private:
    ViewportItem* m_item{nullptr};
    std::unique_ptr<Render::GLRenderer> m_renderer;
    std::unique_ptr<Render::Tessellator> m_tessellator;
    Render::RenderScene m_scene;

    uint64_t m_lastDocumentVersion{0}; ///< Last seen document version
    bool m_needsRebuild{true};
    int m_width{0};
    int m_height{0};
};

/**
 * @brief QML item providing a 3D viewport for geometry visualization
 *
 * This item renders the contents of the GeometryDocument using OpenGL
 * and supports interactive camera controls and entity picking.
 */
class ViewportItem : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(SelectionMode selectionMode READ selectionMode WRITE setSelectionMode NOTIFY
                   selectionModeChanged)
    Q_PROPERTY(bool showFaces READ showFaces WRITE setShowFaces NOTIFY showFacesChanged)
    Q_PROPERTY(bool showEdges READ showEdges WRITE setShowEdges NOTIFY showEdgesChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY
                   backgroundColorChanged)

public:
    enum class SelectionMode { None = 0, Vertex = 1, Edge = 2, Face = 3, Solid = 4, Part = 5 };
    Q_ENUM(SelectionMode)

    explicit ViewportItem(QQuickItem* parent = nullptr);
    ~ViewportItem() override;

    /**
     * @brief Create the renderer
     */
    Renderer* createRenderer() const override;

    // Properties
    [[nodiscard]] SelectionMode selectionMode() const { return m_selectionMode; }
    void setSelectionMode(SelectionMode mode);

    [[nodiscard]] bool showFaces() const { return m_showFaces; }
    void setShowFaces(bool show);

    [[nodiscard]] bool showEdges() const { return m_showEdges; }
    void setShowEdges(bool show);

    [[nodiscard]] QColor backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const QColor& color);

    /**
     * @brief Get the camera settings
     */
    [[nodiscard]] const Render::Camera& camera() const { return m_camera; }

    /**
     * @brief Get display settings
     */
    [[nodiscard]] const Render::DisplaySettings& displaySettings() const {
        return m_displaySettings;
    }

    /**
     * @brief Fit camera to view all visible geometry
     */
    Q_INVOKABLE void fitAll();

    /**
     * @brief Reset camera to default view
     */
    Q_INVOKABLE void resetView();

    /**
     * @brief Zoom to a specific entity
     */
    Q_INVOKABLE void zoomToEntity(quint64 entityId);

signals:
    void selectionModeChanged();
    void showFacesChanged();
    void showEdgesChanged();
    void backgroundColorChanged();

    /**
     * @brief Emitted when an entity is picked
     */
    void entityPicked(quint64 entityId, int entityType, qreal worldX, qreal worldY, qreal worldZ);

    /**
     * @brief Emitted when an entity is hovered
     */
    void entityHovered(quint64 entityId, int entityType);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void hoverMoveEvent(QHoverEvent* event) override;

private:
    void handleRotation(const QPointF& delta);
    void handlePan(const QPointF& delta);
    void handleZoom(float delta);
    void updateCameraFromOrbit();

private:
    SelectionMode m_selectionMode{SelectionMode::Face};
    bool m_showFaces{true};
    bool m_showEdges{true};
    QColor m_backgroundColor{51, 51, 64}; // Dark blue-gray

    Render::Camera m_camera;
    Render::DisplaySettings m_displaySettings;

    // Mouse interaction state
    QPointF m_lastMousePos;
    bool m_rotating{false};
    bool m_panning{false};

    // Orbit center for rotation
    std::array<float, 3> m_orbitCenter{0.0f, 0.0f, 0.0f};
    float m_orbitDistance{100.0f};
    float m_orbitAzimuth{45.0f};   // Horizontal angle in degrees
    float m_orbitElevation{30.0f}; // Vertical angle in degrees
};

} // namespace OpenGeoLab::App
