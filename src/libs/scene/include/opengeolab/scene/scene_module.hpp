/**
 * @file scene_module.hpp
 * @brief SceneModule — scene state management module
 *
 * Owns the SceneGraph and exposes scene operations (visibility, etc.)
 * through the Command/Action protocol.
 *
 * Request format: {"module": "scene", "action": "<name>", "param": {...}}
 */

#pragma once

#include <opengeolab/core/module.hpp>
#include <opengeolab/scene/scene_export.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <kangaroo/util/signal.hpp>

#include <vector>

namespace Kangaroo::Util {
class PluginComponentFactory;
} // namespace Kangaroo::Util

namespace OpenGeoLab::Scene {

/**
 * @brief Scene module — owns SceneGraph and delegates to factory-managed IAction singletons.
 *
 * Bridges SceneGraph signals (nodeAdded, nodeRemoved, nodeUpdated) to
 * ModuleBase::dataChanged for the event bus.
 */
class OPENGEOLAB_SCENE_EXPORT SceneModule final : public Core::ModuleBase {
public:
    explicit SceneModule(Kangaroo::Util::PluginComponentFactory& factory);
    ~SceneModule() override;

    /** @brief Access the scene graph owned by this module. */
    [[nodiscard]] SceneGraph& sceneGraph();
    [[nodiscard]] const SceneGraph& sceneGraph() const;

    static constexpr std::string_view MODULE_NAME{"scene"};

private:
    SceneGraph m_sceneGraph;
    std::vector<Kangaroo::Util::ScopedConnection> m_graphConnections;
};

} // namespace OpenGeoLab::Scene
