/**
 * @file selection_pass.hpp
 * @brief Renders all pickable geometry to RG32UI FBO with per-vertex pickId
 */

#pragma once

#include "core/pick_fbo.hpp"
#include "core/shader_program.hpp"
#include "render_pass_base.hpp"

namespace OpenGeoLab::Render {

/**
 * @brief GPU color-picking pass
 *
 * Renders all geometry to an off-screen RG32UI FBO using per-vertex
 * pick IDs. After rendering, the PickFbo can be queried for hit results.
 */
class SelectionPass final : public RenderPassBase {
public:
    void render(const FrameState& state, const GpuBufferManager& buffers) override;

    /** @brief Access the pick FBO for reading pick results (read-only). */
    [[nodiscard]] const PickFbo& pickFbo() const { return m_pickFbo; }

protected:
    bool onInitialize() override;
    void onCleanup() override;

private:
    ShaderProgram m_shader;
    PickFbo m_pickFbo;
    bool m_pickFboInitialized{false};
};

} // namespace OpenGeoLab::Render
