#pragma once

#include "mesh/mesh_types.hpp"

#include <array>

namespace OpenGeoLab::Mesh {

class MeshElement {
public:
    explicit MeshElement(MeshElementType type);

    ~MeshElement() = default;

    MeshElementId elementId() const { return m_id; }
    MeshElementUID elementUID() const { return m_uid; }
    MeshElementType elementType() const { return m_type; }

    // -----------------------------
    // Node access
    // -----------------------------

    uint8_t nodeCount() const noexcept { return nodeCountFromType(m_type); }

    std::vector<MeshNodeId> nodeIds() const noexcept;

    MeshNodeId nodeId(uint8_t i) const noexcept { return m_nodeIds[i]; }

    void setNodeId(uint8_t i, MeshNodeId id) noexcept { m_nodeIds[i] = id; }

    bool isValid() const {
        return m_id != INVALID_MESH_ELEMENT_ID && m_uid != INVALID_MESH_ELEMENT_UID &&
               m_type != MeshElementType::Invalid;
    }

    MeshElementKey elementKey() const { return MeshElementKey(m_id, m_uid, m_type); }

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