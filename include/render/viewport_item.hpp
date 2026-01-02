/**
 * @file viewport_item.hpp
 * @brief QML viewport item for OpenGL rendering
 *
 * Provides a QQuickItem that integrates OpenGL rendering with QML.
 */

#pragma once

#include "render/opengl_renderer.hpp"

#include <QQuickItem>
#include <memory>

namespace OpenGeoLab {
namespace Render {

/**
 * @brief Selection mode for geometry picking
 */
enum class SelectionMode {
    None,   ///< No selection
    Vertex, ///< Select vertices
    Edge,   ///< Select edges
    Face,   ///< Select faces
    Part    ///< Select parts
};

/**
 * @brief QML viewport item for 3D rendering
 *
 * Integrates OpenGLRenderer with QML and handles:
 * - Mouse interaction for view manipulation
 * - Geometry selection/picking
 * - Scene rendering synchronization
 */
class ViewportItem : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool hasGeometry READ hasGeometry NOTIFY geometryChanged)
    Q_PROPERTY(
        int selectionMode READ selectionMode WRITE setSelectionMode NOTIFY selectionModeChanged)
    Q_PROPERTY(int selectedId READ selectedId NOTIFY selectionChanged)

public:
    explicit ViewportItem(QQuickItem* parent = nullptr);
    ~ViewportItem() override;

    /**
     * @brief Check if geometry is loaded
     * @return True if geometry exists
     */
    bool hasGeometry() const;

    /**
     * @brief Get current selection mode
     * @return Selection mode value
     */
    int selectionMode() const { return static_cast<int>(m_selectionMode); }

    /**
     * @brief Set selection mode
     * @param mode Selection mode value
     */
    void setSelectionMode(int mode);

    /**
     * @brief Get selected geometry ID
     * @return Selected entity ID (0 if none)
     */
    int selectedId() const { return m_selectedId; }

    /**
     * @brief Reset view to fit geometry
     */
    Q_INVOKABLE void fitToView();

    /**
     * @brief Reset view to default
     */
    Q_INVOKABLE void resetView();

    /**
     * @brief Clear selection
     */
    Q_INVOKABLE void clearSelection();

signals:
    void geometryChanged();
    void selectionModeChanged();
    void selectionChanged(int id, int type);
    void renderRequested();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;

private slots:
    void handleWindowChanged(QQuickWindow* window);
    void handleBeforeRendering();
    void handleBeforeRenderPassRecording();
    void onGeometryChanged();

private:
    void initializeRenderer();
    void updateGeometry();
    void performPicking(int x, int y);

    std::unique_ptr<OpenGLRenderer> m_renderer;
    size_t m_geometryCallbackId = 0;

    // Mouse interaction state
    bool m_rotating = false;
    bool m_panning = false;
    QPoint m_lastMousePos;

    // Selection state
    SelectionMode m_selectionMode = SelectionMode::None;
    int m_selectedId = 0;
    bool m_pendingPick = false;
    QPoint m_pickPosition;
};

} // namespace Render
} // namespace OpenGeoLab
