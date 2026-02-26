#pragma once

#include "render/render_scene.hpp"

#include "render/pass/geometry_pass.hpp"
#include "render/pass/mesh_pass.hpp"
#include "render/pass/pick_pass.hpp"

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
    GeometryPass m_geometryPass;
    MeshPass m_meshPass;
    PickPass m_pickPass;

    QSize m_viewportSize;
    bool m_initialized{false};
    bool m_pickPassInitialized{false};
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
} // namespace OpenGeoLab::Render
