/**
 * @file mesh_node.cpp
 * @brief MeshNode construction with automatic UID generation.
 */

#include "mesh/mesh_node.hpp"

namespace OpenGeoLab::Mesh {
MeshNode::MeshNode(double x, double y, double z)
    : m_uid(generateMeshElementUID(MeshElementType::Node)), m_position{x, y, z} {}
} // namespace OpenGeoLab::Mesh
