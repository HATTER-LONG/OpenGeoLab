/**
 * @file pick_result.hpp
 * @brief PickResult — resolved pick target
 */

#pragma once

#include <opengeolab/core/entity_tag.hpp>

#include <cstdint>

namespace OpenGeoLab::Render {

/**
 * @brief Resolved GPU pick result identifying a scene sub-entity.
 *
 * Produced by RenderPipeline::pickAt / pickRegion after decoding
 * the raw RG32UI pick-id texture value through PickResolver.
 */
struct PickResult {
    uint32_t shapeId{0};            /**< Scene node (shape) identifier. */
    Core::EntityType entityType{};  /**< Topological entity kind. */
    uint32_t localId{0};            /**< Sub-entity index within the shape. */
    bool valid{false};              /**< True when the result carries a real hit. */
};

} // namespace OpenGeoLab::Render
