/**
 * @file opengl_viewport.hpp
 * @brief QQuickFramebufferObject-based OpenGL viewport for geometry rendering
 *
 * GLViewport provides a QML-integrable 3D viewport that renders geometry
 * using OpenGL. It supports camera manipulation (rotate, pan, zoom) and
 * integrates with RenderSceneController for scene management. The actual rendering
 * is delegated to SceneRenderer from the render module.
 */

#pragma once

#include "render/render_data.hpp"
#include "render/render_scene_controller.hpp"
#include "render/scene_renderer.hpp"
#include "util/signal.hpp"

#include <QMatrix4x4>
#include <QQuickFramebufferObject>
#include <QRectF>
#include <QtQml/qqml.h>
#include <memory>
#include <optional>

namespace OpenGeoLab::App {

class GLViewportRenderer;

/**
 * @brief QML-integrable OpenGL viewport for 3D geometry visualization
 *
 * GLViewport provides:
 * - OpenGL rendering of geometry from RenderSceneController via SceneRenderer
 * - Mouse-based camera manipulation (orbit, pan, zoom)
 * - Integration with the application's render service
 *
 * @note This class handles QML integration and input; rendering logic is
 *       delegated to Render::SceneRenderer.
 */
class GLViewport : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool hasGeometry READ hasGeometry NOTIFY hasGeometryChanged)
    Q_PROPERTY(
        int selectionMode READ selectionMode WRITE setSelectionMode NOTIFY selectionModeChanged)
    Q_PROPERTY(bool pickingEnabled READ pickingEnabled WRITE setPickingEnabled NOTIFY
                   pickingEnabledChanged)
    Q_PROPERTY(bool boxSelecting READ boxSelecting NOTIFY boxSelectingChanged)
    Q_PROPERTY(QRectF boxSelectionRect READ boxSelectionRect NOTIFY boxSelectionRectChanged)

public:
    explicit GLViewport(QQuickItem* parent = nullptr);
    ~GLViewport() override;

    /**
     * @brief Create the renderer for this viewport
     * @return New renderer instance
     */
    Renderer* createRenderer() const override;

    [[nodiscard]] bool hasGeometry() const;

    /**
     * @brief Get current selection mode
     * @return Selection mode as integer (matches Geometry::SelectionMode)
     */
    [[nodiscard]] int selectionMode() const;

    /**
     * @brief Set selection mode for entity picking
     * @param mode Selection mode (0=None, 1=Vertex, 2=Edge, 3=Face, etc.)
     */
    void setSelectionMode(int mode);

    /**
     * @brief Get current camera state for rendering
     * @return Camera state reference
     */
    [[nodiscard]] const Render::CameraState& cameraState() const;

    /**
     * @brief Get render data for rendering
     * @return Document render data reference
     */
    [[nodiscard]] const Render::DocumentRenderData& renderData() const;

    /**
     * @brief Pick entity at screen position
     * @param x Screen X coordinate
     * @param y Screen Y coordinate
     * @return EntityId at position, or INVALID_ENTITY_ID if background
     */
    Q_INVOKABLE quint64 pickEntityAt(int x, int y);

    /**
     * @brief Pick entities within a rectangle
     * @param x1 Top-left X coordinate
     * @param y1 Top-left Y coordinate
     * @param x2 Bottom-right X coordinate
     * @param y2 Bottom-right Y coordinate
     * @return List of unique EntityIds within the rectangle
     */
    Q_INVOKABLE QVariantList pickEntitiesInRect(int x1, int y1, int x2, int y2);

    /**
     * @brief Clear current picked selection (and highlights)
     */
    Q_INVOKABLE void clearPickedSelection();

    /**
     * @brief Deselect a single entity from current selection
     * @param entityId EntityId to remove
     */
    Q_INVOKABLE void deselectEntity(quint64 entity_id);

    /**
     * @brief Get current picked selection
     * @return List of selected entity IDs
     */
    Q_INVOKABLE QVariantList selectedEntities() const;

    /**
     * @brief Whether picking mode is enabled
     */
    [[nodiscard]] bool pickingEnabled() const { return m_pickingEnabled; }

    /**
     * @brief Enable/disable picking mode
     */
    void setPickingEnabled(bool enabled);

    /**
     * @brief Whether a box selection gesture is active
     */
    [[nodiscard]] bool boxSelecting() const { return m_boxSelecting; }

    /**
     * @brief Current box selection rectangle in item coordinates
     */
    [[nodiscard]] QRectF boxSelectionRect() const { return m_boxSelectionRect; }

signals:
    void hasGeometryChanged();
    void geometryChanged();
    void selectionModeChanged();
    void pickingEnabledChanged();
    void hoveredEntityChanged(quint64 entity_id);
    void selectionChanged(QVariantList entity_ids);
    void boxSelectingChanged();
    void boxSelectionRectChanged();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void hoverMoveEvent(QHoverEvent* event) override;
    void hoverLeaveEvent(QHoverEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void onSceneNeedsUpdate();
    void onServiceGeometryChanged();

private:
    /**
     * @brief Perform orbit camera rotation
     * @param dx Horizontal mouse delta
     * @param dy Vertical mouse delta
     */
    void orbitCamera(float dx, float dy);

    /**
     * @brief Perform camera panning
     * @param dx Horizontal mouse delta
     * @param dy Vertical mouse delta
     */
    void panCamera(float dx, float dy);

    /**
     * @brief Perform camera zoom
     * @param delta Zoom amount
     */
    void zoomCamera(float delta);

private:
    Render::CameraState m_cameraState; ///< Local camera state copy

    bool m_hasGeometry{false};
    Geometry::SelectionMode m_selectionMode{
        Geometry::SelectionMode::None}; ///< Current selection mode

    Util::ScopedConnection m_sceneNeedsUpdateConn;
    Util::ScopedConnection m_cameraChangedConn;
    Util::ScopedConnection m_geometryChangedConn;
    Util::ScopedConnection m_highlightChangedConn; ///< Connection to highlight changes

    QPointF m_lastMousePos;                   ///< Last mouse position for delta calculation
    Qt::MouseButtons m_pressedButtons;        ///< Currently pressed mouse buttons
    Qt::KeyboardModifiers m_pressedModifiers; ///< Currently pressed keyboard modifiers

public:
    /// Picking request structure for entity selection
    enum class PickReason : uint8_t {
        Legacy = 0,      ///< Legacy synchronous Q_INVOKABLE picking
        Hover = 1,       ///< Hover preview
        ClickSelect = 2, ///< Click selection
        BoxSelect = 3,   ///< Box selection
        BoxDeselect = 4  ///< Box deselection
    };

    struct PickRequest {
        int m_x{0};
        int m_y{0};
        bool m_isRect{false};
        int m_x2{0};
        int m_y2{0};
        PickReason reason{PickReason::Legacy};
        bool additive{false}; ///< Ctrl modifier at gesture time
    };

private:
    void requestPick(const PickRequest& request);
    void handlePickResult(const PickRequest& request, const std::vector<Geometry::EntityId>& ids);

    void clearPreviewHighlight();
    void setPreviewHighlight(Geometry::EntityId entity_id);
    void setSelectionInternal(const std::vector<Geometry::EntityId>& ids, bool replace);
    void removeFromSelectionInternal(const std::vector<Geometry::EntityId>& ids);

    mutable std::optional<PickRequest> m_pendingPickRequest;
    mutable std::vector<Geometry::EntityId> m_pickResult;

    bool m_pickingEnabled{false};
    bool m_boxSelecting{false};
    QRectF m_boxSelectionRect;
    QPointF m_boxStart;
    Qt::MouseButton m_boxButton{Qt::NoButton};

    Geometry::EntityId m_previewEntityId{Geometry::INVALID_ENTITY_ID};
    std::vector<Geometry::EntityId> m_selectedEntityIds;

    friend class GLViewportRenderer; ///< Renderer needs access to picking state
};

/**
 * @brief OpenGL renderer for GLViewport
 *
 * Handles the actual OpenGL rendering of geometry meshes using SceneRenderer.
 * Created by GLViewport::createRenderer().
 */
class GLViewportRenderer : public QQuickFramebufferObject::Renderer {
public:
    explicit GLViewportRenderer(const GLViewport* viewport);
    ~GLViewportRenderer() override;

    /**
     * @brief Create the framebuffer object
     * @param size Framebuffer size
     * @return New framebuffer object
     */
    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override;

    /**
     * @brief Render the scene
     */
    void render() override;

    /**
     * @brief Synchronize with the viewport item
     * @param item The viewport item
     */
    void synchronize(QQuickFramebufferObject* item) override;

private:
    /**
     * @brief Perform picking render pass and read back entity IDs
     */
    void performPicking();

private:
    const GLViewport* m_viewport{nullptr};                  ///< Parent viewport item
    std::unique_ptr<Render::SceneRenderer> m_sceneRenderer; ///< Rendering component

    bool m_needsDataUpload{false}; ///< Whether mesh data needs uploading

    // Cached render data
    Render::DocumentRenderData m_renderData;
    Render::CameraState m_cameraState;
    QSize m_viewportSize;

    // Picking support
    Geometry::SelectionMode m_selectionMode{Geometry::SelectionMode::None};
    std::unique_ptr<QOpenGLFramebufferObject> m_pickingFbo; ///< FBO for picking render pass
    std::optional<GLViewport::PickRequest> m_pickRequest;   ///< Current pick request
    std::vector<Geometry::EntityId>* m_pickResult{nullptr}; ///< Output for pick results
};

} // namespace OpenGeoLab::App
