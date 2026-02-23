/**
 * @file mesh_node.cpp
 * @brief Implementation of MeshNode construction and ID generation
 */

#include "mesh/mesh_node.hpp"

namespace OpenGeoLab::Mesh {
MeshNode::MeshNode(double x, double y, double z)
    : m_uid(generateMeshNodeId()), m_position{x, y, z} {}
} // namespace OpenGeoLab::Mesh
