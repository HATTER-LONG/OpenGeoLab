/**
 * @file pass_manager.cpp
 * @brief Implements the render pass manager.
 */
#include <opengeolab/render/pass_manager.hpp>

#include <algorithm>
#include <utility>

namespace OpenGeoLab::Render {

void PassManager::registerPass(std::string name, std::unique_ptr<IRenderPass> pass, int priority) {
    passes_.push_back(PassEntry{std::move(name), std::move(pass), priority});
    sortByPriority();
}

void PassManager::setPassEnabled(std::string_view name, bool enabled) {
    const auto it = std::find_if(passes_.begin(), passes_.end(),
                                 [name](const PassEntry& entry) { return entry.name == name; });
    if(it != passes_.end()) {
        it->enabled = enabled;
    }
}

bool PassManager::isPassEnabled(std::string_view name) const {
    const auto it = std::find_if(passes_.cbegin(), passes_.cend(),
                                 [name](const PassEntry& entry) { return entry.name == name; });
    return it != passes_.cend() ? it->enabled : false;
}

void PassManager::setupAll(int width, int height) {
    for(const auto& entry : passes_) {
        entry.pass->setup(width, height);
    }
}

void PassManager::executeAll(const RenderContext& ctx) {
    for(const auto& entry : passes_) {
        if(entry.enabled) {
            entry.pass->execute(ctx);
        }
    }
}

void PassManager::teardownAll() {
    for(const auto& entry : passes_) {
        entry.pass->teardown();
    }
}

std::size_t PassManager::passCount() const { return passes_.size(); }

void PassManager::sortByPriority() {
    std::sort(passes_.begin(), passes_.end(), [](const PassEntry& lhs, const PassEntry& rhs) {
        return lhs.priority < rhs.priority;
    });
}

} // namespace OpenGeoLab::Render
