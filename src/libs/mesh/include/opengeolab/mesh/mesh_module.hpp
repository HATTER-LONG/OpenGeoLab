/**
 * @file mesh_module.hpp
 * @brief MeshModule — mesh generation and query module
 */

#pragma once

#include <opengeolab/core/module.hpp>
#include <opengeolab/mesh/mesh_export.hpp>
#include <opengeolab/mesh/mesh_store.hpp>

#include <memory>

namespace Kangaroo::Util {
class PluginComponentFactory;
} // namespace Kangaroo::Util

namespace OpenGeoLab::Geometry {
class ShapeStore;
} // namespace OpenGeoLab::Geometry

namespace OpenGeoLab::Scene {
class SceneGraph;
} // namespace OpenGeoLab::Scene

namespace OpenGeoLab::Mesh {

class MeshSceneBridge;

class OPENGEOLAB_MESH_EXPORT MeshModule final : public Core::ModuleBase {
public:
    explicit MeshModule(Kangaroo::Util::PluginComponentFactory& factory);
    ~MeshModule() override;

    [[nodiscard]] MeshStore& meshStore();
    [[nodiscard]] const MeshStore& meshStore() const;

    void initBridge(Scene::SceneGraph& scene, Geometry::ShapeStore& store);

    static constexpr std::string_view MODULE_NAME{"mesh"};

private:
    MeshStore m_meshStore;
    std::unique_ptr<MeshSceneBridge> m_bridge;
};

} // namespace OpenGeoLab::Mesh
