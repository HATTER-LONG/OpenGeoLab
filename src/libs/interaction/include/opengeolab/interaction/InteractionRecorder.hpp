/**
 * @file InteractionRecorder.hpp
 * @brief Records semantic interaction results and exports replay-friendly artifacts.
 */

#pragma once

#include <opengeolab/interaction/InteractionExport.hpp>

#include <nlohmann/json.hpp>

namespace OpenGeoLab::Interaction
{

/**
 * @brief Converts UI-originated interaction outcomes into stable recording, replay, and script artifacts.
 */
class OPENGEOLAB_INTERACTION_EXPORT InteractionRecorder
{
public:
    /**
     * @brief Normalizes one or more interaction operations into semantic recording entries.
     * @param payload JSON payload containing an operation or an operations array.
     * @return JSON recording container for the supplied operations.
     */
    [[nodiscard]] static nlohmann::json recordOperation(const nlohmann::json& payload);

    /**
     * @brief Exports one or more operations to a replay-oriented Python script.
     * @param payload JSON payload containing an operation or an operations array.
     * @return JSON response containing the generated Python script.
     */
    [[nodiscard]] static nlohmann::json exportPythonScript(const nlohmann::json& payload);

    /**
     * @brief Produces an explicit replay plan for one or more recorded operations.
     * @param payload JSON payload containing an operation or an operations array.
     * @return JSON replay plan with stable step boundaries.
     */
    [[nodiscard]] static nlohmann::json describeReplayPlan(const nlohmann::json& payload);
};

}  // namespace OpenGeoLab::Interaction
