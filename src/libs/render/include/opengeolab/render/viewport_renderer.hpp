/**
 * @file viewport_renderer.hpp
 * @brief QQuickFramebufferObject::Renderer for the 3D viewport.
 */

#pragma once

#include <opengeolab/render/camera.hpp>
#include <opengeolab/render/render_engine.hpp>
#include <opengeolab/render/render_scene.hpp>

#include <QOpenGLFramebufferObject>
#include <QQuickFramebufferObject>

namespace OpenGeoLab::Render {

/// Render-thread renderer paired with ViewportItem.
///
/// synchronize() copies Camera state and drains SceneGraph changesets.
/// render() calls RenderEngine to draw the scene.
class ViewportRenderer : public QQuickFramebufferObject::Renderer {
public:
    ViewportRenderer();

    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override;

    void synchronize(QQuickFramebufferObject* item) override;

    void render() override;

private:
    RenderEngine m_engine;
    RenderScene m_renderScene;
    Camera m_cameraCopy;
    int m_width{0};
    int m_height{0};
};

} // namespace OpenGeoLab::Render
