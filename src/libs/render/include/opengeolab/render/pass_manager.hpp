/**
 * @file pass_manager.hpp
 * @brief Declares the PassManager that registers and dispatches render passes.
 */
#pragma once

#include <opengeolab/render/i_render_pass.hpp>
#include <opengeolab/render/render_export.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief Pass registry and execution dispatcher.
 *
 * Executes enabled passes in ascending priority order.
 * Lower priority numbers execute first.
 */
class OPENGEOLAB_RENDER_EXPORT PassManager {
public:
    PassManager() = default;
    ~PassManager() = default;
    PassManager(const PassManager&) = delete;
    PassManager& operator=(const PassManager&) = delete;
    PassManager(PassManager&&) noexcept = default;
    PassManager& operator=(PassManager&&) noexcept = default;

    void registerPass(std::string name, std::unique_ptr<IRenderPass> pass, int priority);
    void setPassEnabled(std::string_view name, bool enabled);
    [[nodiscard]] bool isPassEnabled(std::string_view name) const;

    /** @brief Call setup on all registered passes. */
    void setupAll(int width, int height);

    /** @brief Execute all enabled passes in priority order. */
    void executeAll(const RenderContext& ctx);

    /** @brief Call teardown on all registered passes. */
    void teardownAll();

    [[nodiscard]] std::size_t passCount() const;

private:
    struct PassEntry {
        std::string name;
        std::unique_ptr<IRenderPass> pass;
        int priority;
        bool enabled = true;
    };
    std::vector<PassEntry> passes_;

    void sortByPriority();
};

} // namespace OpenGeoLab::Render
