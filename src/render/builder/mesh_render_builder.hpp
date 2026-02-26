#pragma once

#include "render/render_data.hpp"

#include "mesh/mesh_element.hpp"
#include "mesh/mesh_node.hpp"

#include <span>

namespace OpenGeoLab::Render {

struct MeshRenderInput {
    std::span<const Mesh::MeshNode> m_nodes;
    std::span<const Mesh::MeshElement> m_elements;
};

class MeshRenderBuilder {
public:
    static bool build(RenderData& render_data, const MeshRenderInput& input);
};

} // namespace OpenGeoLab::Render
