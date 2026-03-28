/**
 * @file geometry_module.hpp
 * @brief GeometryModule — geometry creation and manipulation
 *
 * Request format: {"module": "geometry", "action": "<name>", "param": {...}}
 */

#pragma once

#include <opengeolab/core/module.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

namespace Kangaroo::Util {
class PluginComponentFactory;
} // namespace Kangaroo::Util

namespace OpenGeoLab::Geometry {

/**
 * @brief Geometry module — delegates to factory-managed IAction singletons.
 *
 * Actions are registered during construction via registerAction<T>().
 */
class OPENGEOLAB_GEOMETRY_EXPORT GeometryModule final : public Core::ModuleBase {
public:
    explicit GeometryModule(Kangaroo::Util::PluginComponentFactory& factory);
    ~GeometryModule() override;

    static constexpr std::string_view MODULE_NAME{"geometry"};
};

} // namespace OpenGeoLab::Geometry
