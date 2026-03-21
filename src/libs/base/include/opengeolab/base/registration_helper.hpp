/**
 * @file registration_helper.hpp
 * @brief Provides header-only helpers for registering OpenGeoLab module and action components.
 */

#pragma once

#include <opengeolab/base/action_interface.hpp>
#include <opengeolab/base/module_service_interface.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <concepts>
#include <string_view>

namespace OpenGeoLab::Base {

/**
 * @brief Registers a module service as a singleton factory entry.
 * @tparam ModuleT Concrete module type implementing IModuleService.
 * @param factory Component factory receiving the registration.
 * @param module_name Stable module identifier used for lookup.
 * @note ModuleT is constructed with the same factory reference passed to this helper.
 */
template <typename ModuleT>
void registerModule(Kangaroo::Util::PluginComponentFactory& factory, std::string_view module_name) {
    static_assert(std::derived_from<ModuleT, IModuleService>);

    Kangaroo::Util::FactoryRegistration registration{};
    registration.m_interfaceId = Kangaroo::Util::PluginComponentInterfaceId<IModuleService>::VALUE;
    registration.m_moduleName = module_name.data();
    registration.m_factoryContext = &factory;
    registration.m_lifetime = Kangaroo::Util::ComponentFactoryLifetime::Singleton;
    registration.m_createComponent = [](void* factory_context,
                                        Kangaroo::Util::ComponentCreateRequest) noexcept -> void* {
        auto* module_factory =
            static_cast<Kangaroo::Util::PluginComponentFactory*>(factory_context);
        return new ModuleT(*module_factory);
    };
    registration.m_destroyComponent = [](void*, void* object) noexcept {
        delete static_cast<ModuleT*>(object);
    };
    factory.registerFactory(registration);
}

/**
 * @brief Registers an action as a transient factory entry using default construction.
 * @tparam ActionT Concrete action type implementing IAction.
 * @param factory Component factory receiving the registration.
 * @param action_name Stable action identifier used for lookup.
 */
template <typename ActionT>
void registerAction(Kangaroo::Util::PluginComponentFactory& factory, std::string_view action_name) {
    static_assert(std::derived_from<ActionT, IAction>);

    Kangaroo::Util::FactoryRegistration registration{};
    registration.m_interfaceId = Kangaroo::Util::PluginComponentInterfaceId<IAction>::VALUE;
    registration.m_moduleName = action_name.data();
    registration.m_lifetime = Kangaroo::Util::ComponentFactoryLifetime::Transient;
    registration.m_createComponent = [](void*,
                                        Kangaroo::Util::ComponentCreateRequest) noexcept -> void* {
        return new ActionT();
    };
    registration.m_destroyComponent = [](void*, void* object) noexcept {
        delete static_cast<ActionT*>(object);
    };
    factory.registerFactory(registration);
}

/**
 * @brief Registers an action as a transient factory entry using raw Kangaroo callbacks.
 * @tparam ActionT Concrete action type implementing IAction.
 * @param factory Component factory receiving the registration.
 * @param action_name Stable action identifier used for lookup.
 * @param create_component Raw creation callback used for dependency-aware construction.
 * @param destroy_component Raw destruction callback paired with @p create_component.
 * @note The registered callbacks receive the passed factory through the factory context pointer.
 */
template <typename ActionT>
void registerAction(Kangaroo::Util::PluginComponentFactory& factory,
                    std::string_view action_name,
                    decltype(Kangaroo::Util::FactoryRegistration{}.m_createComponent)
                        create_component,
                    decltype(Kangaroo::Util::FactoryRegistration{}.m_destroyComponent)
                        destroy_component) {
    static_assert(std::derived_from<ActionT, IAction>);

    Kangaroo::Util::FactoryRegistration registration{};
    registration.m_interfaceId = Kangaroo::Util::PluginComponentInterfaceId<IAction>::VALUE;
    registration.m_moduleName = action_name.data();
    registration.m_factoryContext = &factory;
    registration.m_lifetime = Kangaroo::Util::ComponentFactoryLifetime::Transient;
    registration.m_createComponent = create_component;
    registration.m_destroyComponent = destroy_component;
    factory.registerFactory(registration);
}

} // namespace OpenGeoLab::Base
