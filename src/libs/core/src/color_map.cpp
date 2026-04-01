#include <opengeolab/core/color_map.hpp>

#include <atomic>

namespace OpenGeoLab::Core::ColorMap {

namespace {

ColorMapConfig g_override = kDefault;
std::atomic<const ColorMapConfig*> g_active{&g_override};

} // namespace

const ColorMapConfig& active() { return *g_active.load(std::memory_order_acquire); }

void setOverride(const ColorMapConfig& config) {
    g_override = config;
    g_active.store(&g_override, std::memory_order_release);
}

} // namespace OpenGeoLab::Core::ColorMap
