/**
 * @file gl_viewport_item.hpp
 * @brief Declares the QML 3D viewport bridging RenderEngine to Qt Scene Graph.
 */
#pragma once

#include <opengeolab/app/viewport_controller.hpp>
#include <opengeolab/render/render_engine.hpp>

#include <QQuickFramebufferObject>
#include <QtQml/qqmlregistration.h>

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
    explicit GLViewportItem(QQuickItem* parent = nullptr);

    [[nodiscard]] Renderer* createRenderer() const override;

    [[nodiscard]] ViewportController* controller() const;
    Render::RenderEngine& renderEngine();

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
