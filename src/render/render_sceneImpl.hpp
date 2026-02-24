#pragma once

#include "render/render_scene.hpp"

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