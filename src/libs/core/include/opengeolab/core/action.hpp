/**
 * @file action.hpp
 * @brief IAction — abstract interface for module actions
 *
 * Each module registers its actions into the global PluginComponentFactory
 * with keys in "module.action" format (e.g. "geometry.create_box").
 */

#pragma once

#include <opengeolab/core/core_export.hpp>
#include <opengeolab/core/progress_callback.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>
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
     *   - "returns"     — success response schema (key → {type, description})
     *
     * Keep the top-level field order stable as:
     *   name, description, params, returns
     *
     * Keep each parameter field order stable as:
     *   type, required, description
     *
     * Keep each return field order stable as:
     *   type, description
     *
     * The "returns" schema must always include:
     *   - "ok"     — boolean success flag
     *   - "action" — echoed action name
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

template <> struct Kangaroo::Util::PluginComponentInterfaceId<OpenGeoLab::Core::IAction> {
    static constexpr std::string_view VALUE{"opengeolab.core.IAction"};
};
