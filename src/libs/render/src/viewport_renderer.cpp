#include <opengeolab/render/viewport_item.hpp>
#include <opengeolab/render/viewport_renderer.hpp>

#include <QOpenGLFramebufferObjectFormat>

namespace OpenGeoLab::Render {

ViewportRenderer::ViewportRenderer() = default;

QOpenGLFramebufferObject* ViewportRenderer::createFramebufferObject(const QSize& size) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);

    m_width = size.width();
    m_height = size.height();

    return new QOpenGLFramebufferObject(size, format);
}

void ViewportRenderer::synchronize(QQuickFramebufferObject* item) {
    auto* viewport = static_cast<ViewportItem*>(item);

    // Copy camera state from main thread
    m_cameraCopy = viewport->camera();

    // Drain changeset from SceneGraph
    auto* graph = viewport->sceneGraph();
    if(graph != nullptr && graph->hasChanges()) {
        auto changeset = graph->consumeChangeset();
        m_renderScene.applyChangeset(changeset, *graph);
    }
}

void ViewportRenderer::render() {
    if(!m_engine.isInitialized()) {
        m_engine.initialize();
    }

    m_engine.resize(m_width, m_height);
    m_engine.render(m_renderScene, m_cameraCopy);

    // Request continuous repaint while there are visible nodes
    if(!m_renderScene.nodes().empty()) {
        update();
    }
}

} // namespace OpenGeoLab::Render
