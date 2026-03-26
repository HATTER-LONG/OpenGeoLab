#pragma once

#include <opengeolab/core/core_export.hpp>
#include <opengeolab/core/progress_callback.hpp>

#include <kangaroo/util/noncopyable.hpp>
#include <kangaroo/util/plugin_component_factory.hpp>
#include <nlohmann/json.hpp>

#include <string>
#include <string_view>

namespace OpenGeoLab::Core {
/**
 * @brief Base class for all modules in OpenGeoLab
 *
 */
class OPENGEOLAB_CORE_EXPORT ModuleBase : public Kangaroo::Util::NonCopyMoveable {
public:
    ModuleBase();
    virtual ~ModuleBase();

    /**
     * @brief Return a JSON description of this module for LLM / tooling.
     *
     * Required fields:
     *   - "name"        — module identifier (matches request["module"])
     *   - "description" — human / LLM-readable summary
     *   - "actions"     — array of action descriptions (from IAction::describe())
     *
     * @return JSON object describing the module and its actions
     */
    [[nodiscard]] virtual nlohmann::json describe() const = 0;

    /**
     * @brief Process a request and return a response
     * @param request Input data as JSON
     * @param progress Callback for reporting progress
     * @return Response data as JSON
     */
    [[nodiscard]] virtual nlohmann::json process(const nlohmann::json& request,
                                                 const ProgressCallback& progress) = 0;
};
} // namespace OpenGeoLab::Core

template <> struct Kangaroo::Util::PluginComponentInterfaceId<OpenGeoLab::Core::ModuleBase> {
    static constexpr std::string_view VALUE{"opengeolab.core.ModuleBase"};
};