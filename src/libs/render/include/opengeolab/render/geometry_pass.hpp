/**
 * @file geometry_pass.hpp
 * @brief Declares the geometry render pass with Phong shading.
 */
#pragma once

#include <opengeolab/render/i_render_pass.hpp>
#include <opengeolab/render/render_export.hpp>
#include <opengeolab/render/shader_program.hpp>
#include <opengeolab/render/vertex_array_object.hpp>
#include <opengeolab/scene/render_mesh_data.hpp>

#include <glm/mat4x4.hpp>

#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief Renders tessellated face geometry with Phong shading.
 *
 * Accepts face meshes (PrimitiveType::Triangles) via setGeometry().
 * Re-uploads VAOs when dirty. Uses directional light aligned to camera.
 */
class OPENGEOLAB_RENDER_EXPORT GeometryPass final : public IRenderPass {
public:
    /** @brief Geometry entry: face mesh + model transform. */
    struct Entry {
        Scene::RenderMeshData faceMesh;
        glm::mat4 transform{1.0F};
    };

    void setup(int width, int height) override;
    void execute(const RenderContext& ctx) override;
    void teardown() override;

    /**
     * @brief Set geometry entries to render.
     * @param entries Face mesh entries with transforms.
     */
    void setGeometry(std::vector<Entry> entries);

private:
    ShaderProgram shader_;
    std::vector<Entry> entries_;
    std::vector<VertexArrayObject> vaos_;
    bool dirty_ = false;
    bool initialized_ = false;
};

} // namespace OpenGeoLab::Render
