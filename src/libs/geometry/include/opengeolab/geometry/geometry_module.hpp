/**
 * @file geometry_module.hpp
 * @brief GeometryModule — a ModuleBase implementation for geometry operations
 *
 * Routes incoming requests to named IAction instances registered in this module.
 * Request format: {"module": "geometry", "action": "<name>", "param": {...}}
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/core/module.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace OpenGeoLab::Geometry {

/**
 * @brief Geometry module with an internal action registry.
 *
 * Each action (e.g. "create_box") is an IAction subclass registered at
 * construction time.  process() extracts the "action" field and delegates.
 */
class OPENGEOLAB_GEOMETRY_EXPORT GeometryModule final : public Core::ModuleBase {
public:
    GeometryModule();
    ~GeometryModule() override;

    /**
     * @brief Describe this module and all registered actions.
     * @return JSON with "name", "description", "actions" array
     */
    [[nodiscard]] nlohmann::json describe() const override;

    /**
     * @brief Dispatch to the IAction matching request["action"].
     * @param request JSON with "action" and "param" fields
     * @param progress Callback for reporting progress
     * @return Result JSON from the action
     * @throws std::invalid_argument if "action" is missing or unknown
     */
    [[nodiscard]] nlohmann::json process(const nlohmann::json& request,
                                         const Core::ProgressCallback& progress) override;

    /**
     * @brief Register an action in this module's action factory.
     * @param action Owning pointer to the action; name must be unique
     */
    void registerAction(std::unique_ptr<Core::IAction> action);

    static constexpr std::string_view MODULE_NAME{"geometry"};

private:
    std::unordered_map<std::string, std::unique_ptr<Core::IAction>> m_actions;
};

} // namespace OpenGeoLab::Geometry
