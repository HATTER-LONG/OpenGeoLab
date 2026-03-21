/**
 * @file geometry_registration.cpp
 * @brief Implements explicit registration of geometry module services and action factories.
 */

#include <opengeolab/geometry/geometry_registration.hpp>

#include <opengeolab/base/registration_helper.hpp>
#include <opengeolab/geometry/bounding_box_action.hpp>
#include <opengeolab/geometry/geometry_module.hpp>
#include <opengeolab/geometry/get_stored_bbox_action.hpp>
#include <opengeolab/geometry/set_points_action.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <memory>
#include <utility>

namespace OpenGeoLab::Geometry {

void registerGeometryModule(Kangaroo::Util::PluginComponentFactory& factory) {
    Base::registerModule<GeometryModule>(factory, "geometry");
    Base::registerAction<BoundingBoxAction>(factory, "geometry.bounding_box");
    Base::registerAction<SetPointsAction>(
        factory, "geometry.set_points",
        [](void*, Kangaroo::Util::ComponentCreateRequest request) noexcept -> void* {
            if(request.m_data == nullptr) {
                return nullptr;
            }
            auto store = *static_cast<const std::shared_ptr<PointStore>*>(request.m_data);
            return new SetPointsAction(std::move(store));
        },
        [](void*, void* object) noexcept { delete static_cast<SetPointsAction*>(object); });
    Base::registerAction<GetStoredBBoxAction>(
        factory, "geometry.get_stored_bbox",
        [](void*, Kangaroo::Util::ComponentCreateRequest request) noexcept -> void* {
            if(request.m_data == nullptr) {
                return nullptr;
            }
            auto store = *static_cast<const std::shared_ptr<PointStore>*>(request.m_data);
            return new GetStoredBBoxAction(std::move(store));
        },
        [](void*, void* object) noexcept { delete static_cast<GetStoredBBoxAction*>(object); });
}

} // namespace OpenGeoLab::Geometry
