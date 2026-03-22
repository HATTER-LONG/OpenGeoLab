/**
 * @file render_registration.cpp
 * @brief Implements explicit registration of render module services and action factories.
 */

#include <opengeolab/render/render_registration.hpp>

#include <opengeolab/base/registration_helper.hpp>
#include <opengeolab/render/actions/add_box_action.hpp>
#include <opengeolab/render/actions/describe_scene_action.hpp>
#include <opengeolab/render/actions/get_camera_state_action.hpp>
#include <opengeolab/render/actions/set_camera_state_action.hpp>
#include <opengeolab/render/actions/set_display_mode_action.hpp>
#include <opengeolab/render/actions/view_all_action.hpp>
#include <opengeolab/render/render_module.hpp>
#include <opengeolab/render/scene_manager.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <memory>
#include <utility>

namespace OpenGeoLab::Render {

namespace {

/// Helper lambda factory for actions that take shared_ptr<SceneManager>.
template <typename ActionT>
auto make_scene_action_callbacks()
    -> std::pair<decltype(Kangaroo::Util::FactoryRegistration{}.m_createComponent),
                 decltype(Kangaroo::Util::FactoryRegistration{}.m_destroyComponent)> {
    return {[](void*, Kangaroo::Util::ComponentCreateRequest request) noexcept -> void* {
                if(request.m_data == nullptr) {
                    return nullptr;
                }
                auto scene_manager =
                    *static_cast<const std::shared_ptr<SceneManager>*>(request.m_data);
                return new ActionT(std::move(scene_manager));
            },
            [](void*, void* object) noexcept { delete static_cast<ActionT*>(object); }};
}

} // namespace

void registerRenderModule(Kangaroo::Util::PluginComponentFactory& factory) {
    Base::registerModule<RenderModule>(factory, "render");

    auto [create_get, destroy_get] = make_scene_action_callbacks<GetCameraStateAction>();
    Base::registerAction<GetCameraStateAction>(factory, "render.camera.get_state", create_get,
                                               destroy_get);

    auto [create_set, destroy_set] = make_scene_action_callbacks<SetCameraStateAction>();
    Base::registerAction<SetCameraStateAction>(factory, "render.camera.set_state", create_set,
                                               destroy_set);

    auto [create_view_all, destroy_view_all] = make_scene_action_callbacks<ViewAllAction>();
    Base::registerAction<ViewAllAction>(factory, "render.camera.view_all", create_view_all,
                                        destroy_view_all);

    auto [create_add_box, destroy_add_box] = make_scene_action_callbacks<AddBoxAction>();
    Base::registerAction<AddBoxAction>(factory, "render.scene.add_box", create_add_box,
                                       destroy_add_box);

    auto [create_describe, destroy_describe] = make_scene_action_callbacks<DescribeSceneAction>();
    Base::registerAction<DescribeSceneAction>(factory, "render.scene.describe", create_describe,
                                              destroy_describe);

    auto [create_set_mode, destroy_set_mode] = make_scene_action_callbacks<SetDisplayModeAction>();
    Base::registerAction<SetDisplayModeAction>(factory, "render.display.set_mode", create_set_mode,
                                               destroy_set_mode);
}

} // namespace OpenGeoLab::Render
