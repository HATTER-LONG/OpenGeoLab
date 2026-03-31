/**
 * @file render_pipeline_detail.hpp
 * @brief Internal helpers for RenderPipeline implementation
 */

#pragma once

#include <opengeolab/render/pick_mask.hpp>

namespace OpenGeoLab::Render::Detail {

/**
 * @brief Convert a pick mask into the resolver mode used by PickResolver.
 *
 * Higher-level modes take precedence so callers can pass combined masks
 * such as PickMask::All and still receive a single resolver strategy.
 */
[[nodiscard]] constexpr PickMode pickModeFromMask(PickMask mask) noexcept {
    if((mask & PickMask::Part) != PickMask::None) {
        return PickMode::Part;
    }

    if((mask & PickMask::Solid) != PickMask::None) {
        return PickMode::Solid;
    }

    if((mask & PickMask::Wire) != PickMask::None) {
        return PickMode::Wire;
    }

    return PickMode::VEF;
}

} // namespace OpenGeoLab::Render::Detail
