/**
 * @file render_pass_base.hpp
 * @brief Common lifecycle base class for render passes.
 */

#pragma once

namespace OpenGeoLab::Render {

/**
 * @brief Base class that unifies initialize/cleanup state handling.
 */
class RenderPassBase {
public:
    virtual ~RenderPassBase() = default;

    void initialize() {
        if(m_initialized) {
            return;
        }
        m_initialized = onInitialize();
    }

    void cleanup() {
        if(!m_initialized) {
            return;
        }
        onCleanup();
        m_initialized = false;
    }

    [[nodiscard]] bool isInitialized() const { return m_initialized; }

protected:
    virtual bool onInitialize() = 0;
    virtual void onCleanup() {}

private:
    bool m_initialized{false};
};

} // namespace OpenGeoLab::Render
