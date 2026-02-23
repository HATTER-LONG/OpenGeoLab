#pragma once
#include "render/render_data.hpp"
#include <QOpenGLFunctions>
#include <QSize>

#include <kangaroo/util/noncopyable.hpp>

namespace OpenGeoLab::Render {

class RenderCore : protected QOpenGLFunctions, public Kangaroo::Util::NonCopyMoveable {
public:
    RenderCore();
    ~RenderCore();

    void initialize();

    bool isInitialized() const { return m_initialized; }

    void cleanup();

    [[nodiscard]] bool checkGLCapabilities() const;

    void setViewportSize(const QSize& size);

    [[nodiscard]] const QSize& viewportSize() const { return m_viewportSize; }

    void uploadMeshData(const DocumentRenderData& data);

private:
    bool m_initialized{false};
    QSize m_viewportSize{800, 600};
};

} // namespace OpenGeoLab::Render