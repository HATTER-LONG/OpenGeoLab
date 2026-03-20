/**
 * @file RenderService.hpp
 * @brief Provides normalized viewport and snapshot contracts for replayable render workflows.
 */

#pragma once

#include <opengeolab/render/RenderExport.hpp>

#include <nlohmann/json.hpp>

namespace OpenGeoLab::Render
{

/**
 * @brief Provides lightweight render-oriented contracts for viewport state and snapshots.
 */
class OPENGEOLAB_RENDER_EXPORT RenderService
{
public:
    /**
     * @brief Normalizes a viewport description into an explicit replayable view state.
     * @param payload JSON payload describing viewport and camera attributes.
     * @return JSON viewport state suitable for scripting and replay planning.
     */
    [[nodiscard]] static nlohmann::json describeViewport(const nlohmann::json& payload);

    /**
     * @brief Produces a placeholder snapshot payload paired with the normalized viewport state.
     * @param payload JSON payload describing viewport and capture intent.
     * @return JSON snapshot container and replay metadata.
     */
    [[nodiscard]] static nlohmann::json captureSnapshot(const nlohmann::json& payload);
};

}  // namespace OpenGeoLab::Render
