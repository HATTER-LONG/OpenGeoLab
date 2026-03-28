/**
 * @file geometry_module.hpp
 * @brief GeometryModule — geometry creation and manipulation
 *
 * Request format: {"module": "geometry", "action": "<name>", "param": {...}}
 */

#pragma once

#include <opengeolab/core/module.hpp>
#include <opengeolab/geometry/geometry_export.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <kangaroo/util/signal.hpp>

#include <vector>

namespace Kangaroo::Util {
class PluginComponentFactory;
} // namespace Kangaroo::Util

namespace OpenGeoLab::Geometry {

/**
 * @brief Geometry module — owns ShapeStore and delegates to factory-managed IAction singletons.
 *
 * Actions are registered during construction via registerAction<T>(ShapeStore&).
 */
class OPENGEOLAB_GEOMETRY_EXPORT GeometryModule final : public Core::ModuleBase {
public:
    explicit GeometryModule(Kangaroo::Util::PluginComponentFactory& factory);
    ~GeometryModule() override;

    /// Access the geometry shape store.
    [[nodiscard]] ShapeStore& shapeStore();
    [[nodiscard]] const ShapeStore& shapeStore() const;

    static constexpr std::string_view MODULE_NAME{"geometry"};

private:
    ShapeStore m_shapeStore;
    std::vector<Kangaroo::Util::ScopedConnection> m_storeConnections;
};

} // namespace OpenGeoLab::Geometry
