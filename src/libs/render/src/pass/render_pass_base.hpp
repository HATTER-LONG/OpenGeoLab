/**
 * @file render_pass_base.hpp
 * @brief Abstract base for render passes
 */

#pragma once

#include <opengeolab/render/frame_state.hpp>

namespace OpenGeoLab::Render {

class GpuBufferManager;

class RenderPassBase {
public:
    virtual ~RenderPassBase() = default;

    void initialize() {
        if(!m_initialized) {
            m_initialized = onInitialize();
        }
    }

    void cleanup() {
        if(m_initialized) {
            onCleanup();
            m_initialized = false;
        }
    }

    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    virtual void render(const FrameState& state, const GpuBufferManager& buffers) = 0;

protected:
    virtual bool onInitialize() = 0;
    virtual void onCleanup() {}

private:
    bool m_initialized{false};
};

} // namespace OpenGeoLab::Render
