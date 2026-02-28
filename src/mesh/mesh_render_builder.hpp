/**
 * @file mesh_render_builder.hpp
 * @brief Converts FEM mesh nodes and elements into GPU-ready render data.
 */

#pragma once

#include "render/render_data.hpp"

#include "mesh/mesh_element.hpp"
#include "mesh/mesh_node.hpp"

#include <span>

namespace OpenGeoLab::Mesh {

/** @brief Input parameters for mesh render data generation. */
struct MeshRenderInput {
    std::span<const MeshNode> m_nodes;       ///< Mesh nodes with positions
    std::span<const MeshElement> m_elements; ///< Mesh elements with connectivity
};

/**
 * @brief Builds GPU render data from FEM mesh nodes and elements.
 *
 * Generates three vertex buffer sections: surface triangles, wireframe edges,
 * and node points. Each element gets a unique pick ID for GPU picking.
 */
class MeshRenderBuilder {
public:
    /**
     * @brief Build render data from mesh nodes and elements.
     * @param render_data Output container for vertices.
     * @param input Mesh nodes, elements, and surface color.
     * @return true on success.
     */
    static bool build(Render::RenderData& render_data, const MeshRenderInput& input);
};

} // namespace OpenGeoLab::Mesh
