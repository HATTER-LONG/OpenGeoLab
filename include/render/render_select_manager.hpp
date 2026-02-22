#pragma once

#include "geometry/geometry_types.hpp"
#include "mesh/mesh_types.hpp"
#include "render/pick_entity_type.hpp"
#include "util/signal.hpp"

#include <cstdint>
#include <kangaroo/util/noncopyable.hpp>

namespace OpenGeoLab::Render {

struct PickResult {
    uint32_t m_uid;        ///< Type-scoped unique identifier, Note: MeshNode only has node id
    PickEntityType m_type; ///< Entity type for selection filtering

    Mesh::MeshElementType m_meshElementType{
        Mesh::MeshElementType::Invalid}; ///< Mesh element type (if applicable)
    bool operator==(const PickResult& other) const noexcept {
        if(m_uid != other.m_uid) {
            return false;
        }
        if(m_type != other.m_type) {
            return false;
        }
        if(m_type == PickEntityType::MeshElement) {
            return m_meshElementType == other.m_meshElementType;
        }
        return true;
    }
};

struct PickResultHash {
    static constexpr void hashCombine(size_t& seed, size_t value) {
        seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    }
    std::size_t operator()(const PickResult& result) const noexcept {
        std::size_t seed = 0;
        hashCombine(seed, result.m_uid);
        hashCombine(seed, static_cast<std::underlying_type_t<PickEntityType>>(result.m_type));
        if(result.m_type == PickEntityType::MeshElement) {
            hashCombine(seed, static_cast<std::underlying_type_t<Mesh::MeshElementType>>(
                                  result.m_meshElementType));
        }
        return seed;
    }
};

using PickResultSet = std::unordered_set<PickResult, PickResultHash>;

enum class SelectionChangeAction : uint8_t { Added = 0, Removed = 1, Cleared = 2 };

class RenderSelectManager : public Kangaroo::Util::NonCopyMoveable {
private:
    RenderSelectManager() = default;

public:
    static RenderSelectManager& instance();
    ~RenderSelectManager() = default;

    void setPickEnabled(bool enabled);
    bool isPickEnabled() const;

    void setPickTypes(PickMask types);

    PickMask getPickTypes() const;

    bool isTypePickable(PickEntityType type) const;

    PickMask normalizePickTypes(PickMask types);

    bool addSelection(Geometry::EntityUID entity_uid, Geometry::EntityType type);

    bool removeSelection(Geometry::EntityUID entity_uid, Geometry::EntityType type);

    bool addSelection(Mesh::MeshElementUID element_id, Mesh::MeshElementType type);

    bool removeSelection(Mesh::MeshElementUID element_id, Mesh::MeshElementType type);

    bool addSelection(Mesh::MeshNodeId node_id);

    bool removeSelection(Mesh::MeshNodeId node_id);

    void clearSelection();

    std::vector<PickResult> selections() const;

    Util::ScopedConnection subscribePickEnabledChanged(std::function<void(bool)> callback) {
        return m_pickEnabledChanged.connect(std::move(callback));
    }

    Util::ScopedConnection subscribePickSettingsChanged(std::function<void(PickMask)> callback) {
        return m_pickSettingsChanged.connect(std::move(callback));
    }

    Util::ScopedConnection
    subscribeSelectionChanged(std::function<void(PickResult, SelectionChangeAction)> callback) {
        return m_selectionChanged.connect(std::move(callback));
    }

private:
    bool modifySelection(SelectionChangeAction action, PickResult result);

private:
    mutable std::mutex m_mutex;
    bool m_pickEnabled{false};
    PickMask m_pickTypes{PickMask::None};

    PickResultSet m_currentSelections;

    // Geometry::EntityRefSet m_selectedGeometryEntities;

    // Mesh::MeshElementRefSet m_selectedMeshElements;
    // Mesh::MeshNodeIdSet m_selectedMeshNodes;

    Util::Signal<PickMask> m_pickSettingsChanged;
    Util::Signal<bool> m_pickEnabledChanged;
    Util::Signal<PickResult, SelectionChangeAction> m_selectionChanged;
};
} // namespace OpenGeoLab::Render