/**
 * @file gl_viewport_item.hpp
 * @brief Declares the QML 3D viewport bridging RenderEngine to Qt Scene Graph.
 */
#pragma once

#include <opengeolab/app/viewport_controller.hpp>
#include <opengeolab/render/render_engine.hpp>

#include <QQuickFramebufferObject>
#include <QtQml/qqmlregistration.h>

namespace OpenGeoLab::Scene {
class SceneGraph;
}

namespace OpenGeoLab::App {

/**
 * @brief QML 3D viewport component bridging RenderEngine and Qt Scene Graph.
 *
 * Inherits QQuickFramebufferObject. The Renderer subclass (defined in .cpp)
 * calls RenderEngine in Qt's render thread.
 */
class GLViewportItem : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(ViewportController* controller READ controller CONSTANT)

public:
    /**
     * @brief Construct the viewport item and its viewport controller.
     * @param parent Optional parent quick item.
     */
    explicit GLViewportItem(QQuickItem* parent = nullptr);

    /** @brief Create the Qt scene graph renderer bound to this viewport item. */
    [[nodiscard]] Renderer* createRenderer() const override;

    /** @brief Access the controller that translates user input into camera commands. */
    [[nodiscard]] ViewportController* controller() const;

    /** @brief Access the render engine owned by this viewport item. */
    Render::RenderEngine& renderEngine();

    /**
     * @brief Attach the scene graph consumed by the render engine.
     * @param graph Scene graph to visualize, or @c nullptr to disconnect.
     */
    void setSceneGraph(Scene::SceneGraph* graph);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    Render::RenderEngine renderEngine_;
    ViewportController* controller_;
};

} // namespace OpenGeoLab::App
