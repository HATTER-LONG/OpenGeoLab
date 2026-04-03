/**
 * @file mesh_node.hpp
 * @brief MeshNode — mesh vertex position
 */

#pragma once

namespace OpenGeoLab::Mesh {

/**
 * @brief A mesh node (vertex) storing its 3D position.
 *
 * Nodes are stored contiguously in MeshEntry::nodes.
 * The localId is implicit: `localId = vector_index + 1` (1-based).
 */
struct MeshNode {
    float position[3]{}; ///< World-space coordinates (x, y, z)
};

} // namespace OpenGeoLab::Mesh
