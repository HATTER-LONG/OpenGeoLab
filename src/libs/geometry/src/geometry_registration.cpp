/**
 * @file geometry_registration.cpp
 * @brief Registers geometry module services and action factories at static initialization time.
 */

#include <opengeolab/base/registration_helper.hpp>
#include <opengeolab/geometry/bounding_box_action.hpp>
#include <opengeolab/geometry/geometry_module.hpp>
#include <opengeolab/geometry/get_stored_bbox_action.hpp>
#include <opengeolab/geometry/set_points_action.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <memory>
#include <utility>

// Exported anchor symbol — referencing it from consumers forces the linker to
// import ogl_geometry, which triggers the static registration lambda below.
extern "C" OPENGEOLAB_GEOMETRY_EXPORT void ogl_geometry_force_load() noexcept {}

namespace {

[[maybe_unused]] const bool kRegistered = []() {
    auto& factory = Kangaroo::Util::PluginComponentFactory::instance();

    OpenGeoLab::Base::registerModule<OpenGeoLab::Geometry::GeometryModule>(factory, "geometry");
    OpenGeoLab::Base::registerAction<OpenGeoLab::Geometry::BoundingBoxAction>(
        factory, "geometry.bounding_box");
    OpenGeoLab::Base::registerAction<OpenGeoLab::Geometry::SetPointsAction>(
        factory, "geometry.set_points",
        [](void*, Kangaroo::Util::ComponentCreateRequest request) noexcept -> void* {
            if(request.m_data == nullptr) {
                return nullptr;
            }

            auto store = *static_cast<const std::shared_ptr<OpenGeoLab::Geometry::PointStore>*>(
                request.m_data);
            return new OpenGeoLab::Geometry::SetPointsAction(std::move(store));
        },
        [](void*, void* object) noexcept {
            delete static_cast<OpenGeoLab::Geometry::SetPointsAction*>(object);
        });

    OpenGeoLab::Base::registerAction<OpenGeoLab::Geometry::GetStoredBBoxAction>(
        factory, "geometry.get_stored_bbox",
        [](void*, Kangaroo::Util::ComponentCreateRequest request) noexcept -> void* {
            if(request.m_data == nullptr) {
                return nullptr;
            }

            auto store = *static_cast<const std::shared_ptr<OpenGeoLab::Geometry::PointStore>*>(
                request.m_data);
            return new OpenGeoLab::Geometry::GetStoredBBoxAction(std::move(store));
        },
        [](void*, void* object) noexcept {
            delete static_cast<OpenGeoLab::Geometry::GetStoredBBoxAction*>(object);
        });

    return true;
}();

} // namespace
