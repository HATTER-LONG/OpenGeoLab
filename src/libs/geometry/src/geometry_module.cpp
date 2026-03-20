#include <opengeolab/geometry/geometry_module.hpp>

#include <opengeolab/command/action_interface.hpp>

#include <nlohmann/json.hpp>

#include <string>

namespace OpenGeoLab::Geometry {

GeometryModule::GeometryModule(Kangaroo::Util::PluginComponentFactory& factory)
    : m_factory(factory), m_pointStore(std::make_shared<PointStore>()) {}

GeometryModule::~GeometryModule() = default;

auto GeometryModule::moduleName() const noexcept -> std::string_view { return "geometry"; }

auto GeometryModule::dispatch(std::string_view action, const nlohmann::json& payload)
    -> Command::CommandResult {
    const auto factory_module_name = std::string(moduleName()) + "." + std::string(action);
    const auto request = Kangaroo::Util::ComponentCreateRequest::from(m_pointStore);

    try {
        auto action_ptr = m_factory.create<Command::IAction>(factory_module_name, request);
        return action_ptr->execute(payload);
    } catch(const Kangaroo::Util::ComponentFactoryNotRegisteredEx&) {
        return Command::CommandResult{
            .ok = false,
            .summary = "Unknown action: " + std::string(action),
            .result = nlohmann::json::object(),
        };
    }
}

auto GeometryModule::supportedActions() const -> std::vector<std::string> {
    return {"bounding_box", "set_points", "get_stored_bbox"};
}

} // namespace OpenGeoLab::Geometry
