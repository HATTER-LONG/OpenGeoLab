/// @file generate_surface_mesh_action.hpp
/// @brief Action to generate a 2D surface mesh on an OCC shape via Gmsh.

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/mesh/mesh_export.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

namespace OpenGeoLab::Mesh {

class MeshStore;

/// @brief Generates a 2D surface mesh for a given shape ID.
///
/// Retrieves the OCC shape from GeometryModule's ShapeStore via factory,
/// invokes GmshBridge::generateSurfaceMesh, builds VisualData, and
/// stores the result in MeshStore.
class OPENGEOLAB_MESH_EXPORT GenerateSurfaceMeshAction final : public Core::IAction {
public:
    GenerateSurfaceMeshAction(MeshStore& store, Kangaroo::Util::PluginComponentFactory& factory);
    ~GenerateSurfaceMeshAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"generate_surface_mesh"};

private:
    MeshStore& m_store;
    Kangaroo::Util::PluginComponentFactory& m_factory;
};

} // namespace OpenGeoLab::Mesh
