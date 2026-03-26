/**
 * @file action.hpp
 * @brief IAction — abstract interface for module actions
 *
 * Each module maintains its own action registry.  process() dispatches
 * to the IAction matching the "action" field in the request.
 */

#pragma once

#include <opengeolab/core/core_export.hpp>
#include <opengeolab/core/progress_callback.hpp>

#include <nlohmann/json.hpp>

namespace OpenGeoLab::Core {

/**
 * @brief Abstract action executed inside a module.
 *
 * Implementors handle one specific operation identified by name (e.g. "read_brep").
 */
class OPENGEOLAB_CORE_EXPORT IAction {
public:
    IAction();
    virtual ~IAction();

    /**
     * @brief Return a JSON description of this action for LLM / tooling.
     *
     * Required fields:
     *   - "name"        — action identifier (matches request["action"])
     *   - "description" — human / LLM-readable summary
     *   - "params"      — parameter schema (key → {type, required, description})
     *   - "returns"     — return value schema (key → {type, description})
     *
     * @return JSON object describing the action
     */
    [[nodiscard]] virtual nlohmann::json describe() const = 0;

    /**
     * @brief Execute the action.
     * @param param The "param" object from the request JSON
     * @param progress Callback for reporting progress
     * @return Result JSON
     */
    [[nodiscard]] virtual nlohmann::json execute(const nlohmann::json& param,
                                                 const ProgressCallback& progress) = 0;
};

} // namespace OpenGeoLab::Core
