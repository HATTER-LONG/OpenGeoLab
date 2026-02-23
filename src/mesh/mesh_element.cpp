/**
 * @file mesh_element.cpp
 * @brief Implementation of MeshElement construction and UID generation
 */

#include "mesh/mesh_element.hpp"
namespace OpenGeoLab::Mesh {
MeshElement::MeshElement(MeshElementType type)
    : m_id(generateMeshElementId()), m_uid(generateMeshElementUID(type)), m_type(type) {}

std::vector<MeshNodeId> MeshElement::nodeIds() const noexcept {
    return std::vector<MeshNodeId>(m_nodeIds.begin(), m_nodeIds.begin() + nodeCount());
}

} // namespace OpenGeoLab::Mesh