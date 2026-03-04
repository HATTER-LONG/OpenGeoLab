#include "mesh/mesh_element.hpp"
namespace OpenGeoLab::Mesh {
MeshElement::MeshElement(MeshElementType type, Geometry::EntityUID source_part_uid)
    : m_id(generateMeshElementId()), m_uid(generateMeshElementUID(type)), m_type(type),
      m_sourcePartUid(source_part_uid) {}

std::vector<MeshNodeId> MeshElement::nodeIds() const noexcept {
    return std::vector<MeshNodeId>(m_nodeIds.begin(), m_nodeIds.begin() + nodeCount());
}

} // namespace OpenGeoLab::Mesh