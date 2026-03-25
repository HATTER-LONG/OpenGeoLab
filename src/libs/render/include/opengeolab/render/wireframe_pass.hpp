/**
 * @file wireframe_pass.hpp
 * @brief Declares the wireframe edge overlay render pass.
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
 * @brief Renders edge wireframe overlay on top of solid geometry.
 *
 * Accepts edge meshes (PrimitiveType::Lines) via setGeometry().
 * Uses polygon offset to prevent z-fighting with GeometryPass.
 */
class OPENGEOLAB_RENDER_EXPORT WireframePass final : public IRenderPass {
public:
    /** @brief Edge mesh entry with model transform. */
    struct Entry {
        Scene::RenderMeshData edgeMesh;
        glm::mat4 transform{1.0F};
    };

    /**
     * @brief Initialize shaders and GL state for wireframe rendering.
     * @param width Current framebuffer width in pixels.
     * @param height Current framebuffer height in pixels.
     */
    void setup(int width, int height) override;

    /**
     * @brief Render the configured edge meshes for the current frame.
     * @param ctx Frame render context containing matrices and viewport state.
     */
    void execute(const RenderContext& ctx) override;

    /** @brief Release GL resources owned by the wireframe pass. */
    void teardown() override;

    /**
     * @brief Set edge geometry entries to render.
     * @param entries Edge mesh entries with transforms.
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
