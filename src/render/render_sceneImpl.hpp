#pragma once

#include "render/render_data.hpp"
#include "render/render_scene.hpp"

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

#include <array>
#include <cstdint>

namespace OpenGeoLab::Render {
class RenderSceneImpl : public IRenderScene {
public:
    RenderSceneImpl();
    ~RenderSceneImpl() override;

    void initialize() override;
    [[nodiscard]] bool isInitialized() const override;
    void setViewportSize(const QSize& size) override;
    void processPicking(const PickingInput& input) override;
    void render(const QVector3D& camera_pos,
                const QMatrix4x4& view_matrix,
                const QMatrix4x4& projection_matrix) override;
    void cleanup() override;

private:
    struct GpuTopologyBucket {
        QOpenGLVertexArrayObject m_vao;
        QOpenGLBuffer m_vbo{QOpenGLBuffer::VertexBuffer};
        QOpenGLBuffer m_ebo{QOpenGLBuffer::IndexBuffer};
        int m_indexCount{0};
        bool m_created{false};
    };

    void ensureGpuResources();
    bool ensurePickResources();
    bool ensurePickFramebuffer(const QSize& size);
    void uploadBuckets(const RenderBucket& bucket, RenderDisplayMode mode);
    void drawBuckets(const QMatrix4x4& mvp);
    void drawBuckets(const QMatrix4x4& mvp,
                     QOpenGLShaderProgram& shader,
                     std::array<GpuTopologyBucket, 3>& buckets);
    void releaseGpuResources();
    void releasePickFramebuffer();

    static int topologyIndex(PrimitiveTopology topology);

    bool m_initialized{false};
    QSize m_viewportSize;
    bool m_gpuReady{false};
    QOpenGLShaderProgram m_shader;
    QOpenGLShaderProgram m_pickShader;
    std::array<GpuTopologyBucket, 3> m_topologyBuckets;
    std::array<GpuTopologyBucket, 3> m_pickTopologyBuckets;

    uint32_t m_pickFramebuffer{0};
    uint32_t m_pickColorTexture{0};
    uint32_t m_pickDepthRenderbuffer{0};
    QSize m_pickBufferSize;

    uint64_t m_lastUploadedRevision{0};
    bool m_hasUploadedData{false};
    RenderDisplayMode m_lastUploadedMode{RenderDisplayMode::Surface};
};

// =============================================================================
// SceneRendererFactory implementation
// =============================================================================

class SceneRendererFactoryImpl : public SceneRendererFactory {
public:
    SceneRendererFactoryImpl() = default;
    ~SceneRendererFactoryImpl() = default;

    tObjectPtr create() override { return std::make_unique<RenderSceneImpl>(); }
};

// void registerSceneRendererFactory() {
// }
} // namespace OpenGeoLab::Render