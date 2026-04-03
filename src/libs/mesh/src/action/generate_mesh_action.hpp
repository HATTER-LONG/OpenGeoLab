/**
 * @file generate_mesh_action.hpp
 * @brief GenerateMeshAction — mesh OCC faces or solids with gmsh
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/mesh/mesh_export.hpp>

namespace OpenGeoLab::Geometry {
class ShapeStore;
}

namespace OpenGeoLab::Mesh {

class MeshStore;

class OPENGEOLAB_MESH_EXPORT GenerateMeshAction final : public Core::IAction {
public:
    GenerateMeshAction(MeshStore& mesh_store, Geometry::ShapeStore& shape_store);
    ~GenerateMeshAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "generate_mesh";

private:
    MeshStore& m_meshStore;
    Geometry::ShapeStore& m_shapeStore;
};

} // namespace OpenGeoLab::Mesh
