/**
 * @file mesh_element.hpp
 * @brief Mesh element representation with type-specific node connectivity
 */

#pragma once

#include "mesh/mesh_types.hpp"

#include <array>
#include <kangaroo/util/noncopyable.hpp>

namespace OpenGeoLab::Mesh {

/**
 * @brief Represents a single finite-element with typed node connectivity.
 *
 * Each MeshElement has a topology (MeshElementType) that determines the
 * fixed number of nodes it references. The element is move-only; copy
 * construction and assignment are deleted.
 */
class MeshElement {
public:
    /**
     * @brief Construct an element of the given topology.
     * @param type The finite-element topology (e.g. Triangle, Hexa8).
     *             Determines the node count returned by nodeCount().
     * @note A global MeshElementId and a type-scoped MeshElementUID are
     *       auto-generated during construction.
     * @warning @p type must not be MeshElementType::Invalid.
     */
    explicit MeshElement(MeshElementType type);

    ~MeshElement() = default;

    MeshElement(MeshElement&&) noexcept = default;
    MeshElement& operator=(MeshElement&&) noexcept = default;
    MeshElement(const MeshElement&) = delete;
    MeshElement& operator=(const MeshElement&) = delete;

    MeshElementId elementId() const { return m_id; }
    MeshElementUID elementUID() const { return m_uid; }
    MeshElementType elementType() const { return m_type; }

    // -----------------------------
    // Node access
    // -----------------------------

    uint8_t nodeCount() const noexcept { return nodeCountFromType(m_type); }

    /**
     * @brief Return a copy of the first nodeCount() node identifiers.
     * @return A vector whose size equals nodeCount().
     */
    std::vector<MeshNodeId> nodeIds() const noexcept;

    MeshNodeId nodeId(uint8_t i) const noexcept { return m_nodeIds[i]; }

    void setNodeId(uint8_t i, MeshNodeId id) noexcept { m_nodeIds[i] = id; }

    /**
     * @brief Check whether this element has been fully initialised.
     * @return true if the element carries a valid ID, UID, and non-Invalid type.
     */
    bool isValid() const {
        return m_id != INVALID_MESH_ELEMENT_ID && m_uid != INVALID_MESH_ELEMENT_UID &&
               m_type != MeshElementType::Invalid;
    }

    /**
     * @brief Build a full identity key (ID + UID + type) for this element.
     * @return A MeshElementKey that uniquely identifies this element globally.
     */
    MeshElementKey elementKey() const { return MeshElementKey(m_id, m_uid, m_type); }

    /**
     * @brief Build a lightweight reference (UID + type) for this element.
     * @return A MeshElementRef that identifies this element within its type scope.
     */
    MeshElementRef elementRef() const { return MeshElementRef(m_uid, m_type); }

private:
    static constexpr uint8_t nodeCountFromType(MeshElementType type) noexcept {
        switch(type) {
        case MeshElementType::Line:
            return 2;
        case MeshElementType::Triangle:
            return 3;
        case MeshElementType::Quad4:
            return 4;
        case MeshElementType::Tetra4:
            return 4;
        case MeshElementType::Hexa8:
            return 8;
        case MeshElementType::Prism6:
            return 6;
        case MeshElementType::Pyramid5:
            return 5;
        default:
            return 0;
        }
    }

private:
    MeshElementId m_id{INVALID_MESH_ELEMENT_ID};
    MeshElementUID m_uid{INVALID_MESH_ELEMENT_UID};
    MeshElementType m_type{MeshElementType::Invalid};

    std::array<MeshNodeId, 8> m_nodeIds{};
};
} // namespace OpenGeoLab::Mesh