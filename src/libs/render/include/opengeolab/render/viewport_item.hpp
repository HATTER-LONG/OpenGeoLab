/**
 * @file viewport_item.hpp
 * @brief QML 3D viewport backed by QQuickFramebufferObject.
 */

#pragma once

#include <opengeolab/render/camera.hpp>
#include <opengeolab/render/render_export.hpp>
#include <opengeolab/render/trackball_controller.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <QQuickFramebufferObject>

namespace OpenGeoLab::Render {

/// QML-visible 3D viewport that renders via QQuickFramebufferObject.
///
/// Mouse interaction mapping (Phase 1 — navigate only):
/// - Ctrl + Left drag → Orbit
/// - Shift + Left drag / Middle drag → Pan
/// - Right drag → Zoom
/// - Scroll wheel → Zoom
class OPENGEOLAB_RENDER_EXPORT ViewportItem : public QQuickFramebufferObject {
    Q_OBJECT

public:
    explicit ViewportItem(QQuickItem* parent = nullptr);

    Renderer* createRenderer() const override;

    void setSceneGraph(Scene::SceneGraph* graph);
    [[nodiscard]] Scene::SceneGraph* sceneGraph() const;

    Camera& camera();
    [[nodiscard]] const Camera& camera() const;

    Q_INVOKABLE void fitAll();
    Q_INVOKABLE void setFrontView();
    Q_INVOKABLE void setBackView();
    Q_INVOKABLE void setLeftView();
    Q_INVOKABLE void setRightView();
    Q_INVOKABLE void setTopView();
    Q_INVOKABLE void setBottomView();

Q_SIGNALS:
    void sceneChanged();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

private:
    Camera m_camera;
    TrackballController m_trackball;
    Scene::SceneGraph* m_sceneGraph{nullptr};
};

} // namespace OpenGeoLab::Render
