/**
 * @file geometry_registration.cpp
 * @brief Registers geometry module services and action factories at static initialization time.
 */

#include <opengeolab/geometry/bounding_box_action.hpp>
#include <opengeolab/geometry/geometry_module.hpp>
#include <opengeolab/geometry/get_stored_bbox_action.hpp>
#include <opengeolab/geometry/set_points_action.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <memory>
#include <utility>

namespace {

void registerAction(Kangaroo::Util::PluginComponentFactory& factory,
                    const char* module_name,
                    decltype(Kangaroo::Util::FactoryRegistration{}.m_createComponent)
                        create_component,
                    decltype(Kangaroo::Util::FactoryRegistration{}.m_destroyComponent)
                        destroy_component) {
    Kangaroo::Util::FactoryRegistration registration{};
    registration.m_interfaceId = "OpenGeoLab.IAction";
    registration.m_moduleName = module_name;
    registration.m_lifetime = Kangaroo::Util::ComponentFactoryLifetime::Transient;
    registration.m_createComponent = create_component;
    registration.m_destroyComponent = destroy_component;
    factory.registerFactory(registration);
}

[[maybe_unused]] const bool kRegistered = []() {
    auto& factory = Kangaroo::Util::PluginComponentFactory::instance();

    Kangaroo::Util::FactoryRegistration module_registration{};
    module_registration.m_interfaceId = "OpenGeoLab.IModuleService";
    module_registration.m_moduleName = "geometry";
    module_registration.m_lifetime = Kangaroo::Util::ComponentFactoryLifetime::Singleton;
    module_registration.m_createComponent =
        [](void*, Kangaroo::Util::ComponentCreateRequest) noexcept -> void* {
        return new OpenGeoLab::Geometry::GeometryModule(
            Kangaroo::Util::PluginComponentFactory::instance());
    };
    module_registration.m_destroyComponent = [](void*, void* object) noexcept {
        delete static_cast<OpenGeoLab::Geometry::GeometryModule*>(object);
    };
    factory.registerFactory(module_registration);

    registerAction(
        factory, "geometry.bounding_box",
        [](void*, Kangaroo::Util::ComponentCreateRequest) noexcept -> void* {
            return new OpenGeoLab::Geometry::BoundingBoxAction();
        },
        [](void*, void* object) noexcept {
            delete static_cast<OpenGeoLab::Geometry::BoundingBoxAction*>(object);
        });

    registerAction(
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

    registerAction(
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
