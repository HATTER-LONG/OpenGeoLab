/**
 * @file SelectionService.hpp
 * @brief Provides semantic pick and box-selection descriptions for replayable workflows.
 */

#pragma once

#include <opengeolab/selection/SelectionExport.hpp>

#include <nlohmann/json.hpp>

namespace OpenGeoLab::Selection
{

/**
 * @brief Provides stable selection query contracts decoupled from transient UI gestures.
 */
class OPENGEOLAB_SELECTION_EXPORT SelectionService
{
public:
    /**
     * @brief Describes a single-point selection query.
     * @param payload JSON payload containing viewport context and screen coordinates.
     * @return JSON description of the normalized pick query.
     */
    [[nodiscard]] static nlohmann::json describePick(const nlohmann::json& payload);

    /**
     * @brief Describes a box-selection query and its replay boundary.
     * @param payload JSON payload containing viewport context, rectangle, and selection filters.
     * @return JSON description of the normalized box-selection query.
     */
    [[nodiscard]] static nlohmann::json describeBoxSelection(const nlohmann::json& payload);
};

}  // namespace OpenGeoLab::Selection
