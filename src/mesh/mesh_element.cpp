#include "mesh/mesh_element.hpp"
namespace OpenGeoLab::Mesh {
MeshElement::MeshElement(MeshElementType type)
    : m_id(generateMeshElementId()), m_uid(generateMeshElementUID(type)), m_type(type) {}
} // namespace OpenGeoLab::Mesh