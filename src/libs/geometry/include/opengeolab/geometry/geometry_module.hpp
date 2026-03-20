/**
 * @file geometry_module.hpp
 * @brief Declares the geometry module service for factory-based action dispatch.
 */

#pragma once

#include <opengeolab/command/module_service_interface.hpp>
#include <opengeolab/geometry/geometry_export.hpp>
#include <opengeolab/geometry/point_store.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace OpenGeoLab::Geometry {

/**
 * @brief Routes geometry actions through the plugin component factory.
 */
class OPENGEOLAB_GEOMETRY_EXPORT GeometryModule : public Command::IModuleService {
public:
    explicit GeometryModule(Kangaroo::Util::PluginComponentFactory& factory);
    ~GeometryModule() override;

    [[nodiscard]] auto moduleName() const noexcept -> std::string_view override;
    [[nodiscard]] auto dispatch(std::string_view action, const nlohmann::json& payload)
        -> Command::CommandResult override;
    [[nodiscard]] auto supportedActions() const -> std::vector<std::string> override;

private:
    Kangaroo::Util::PluginComponentFactory& m_factory;
    std::shared_ptr<PointStore> m_pointStore;
};

} // namespace OpenGeoLab::Geometry
