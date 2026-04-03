/**
 * @file mesh_element.hpp
 * @brief MeshElement — finite element connectivity
 */

#pragma once

#include <opengeolab/mesh/mesh_element_type.hpp>

#include <array>
#include <cstdint>

namespace OpenGeoLab::Mesh {

/**
 * @brief A mesh element storing its topology type and node connectivity.
 *
 * Elements are stored contiguously in MeshEntry::elements.
 * The localId is implicit: `localId = vector_index + 1` (1-based).
 *
 * Only the first `nodeCount(type)` entries in nodeLocalIds are valid.
 */
struct MeshElement {
    MeshElementType type{};                                   ///< Element topology
    std::array<uint32_t, K_MAX_ELEMENT_NODES> nodeLocalIds{}; ///< Node localIds (1-based)
};

} // namespace OpenGeoLab::Mesh
