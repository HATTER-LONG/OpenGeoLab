#pragma once

#include "geometry_entity.hpp"
#include <TopTools_ShapeMapHasher.hxx>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Geometry {
class EntityIndex : public Kangaroo::Util::NonCopyMoveable {
public:
    explicit EntityIndex() = default;
    ~EntityIndex() = default;

    [[nodiscard]] bool addEntity(const GeometryEntityPtr& entity);
    [[nodiscard]] bool removeEntity(EntityId entity_id);
    [[nodiscard]] bool removeEntity(const GeometryEntityPtr& entity);
    [[nodiscard]] bool removeEntity(EntityUID entity_uid, EntityType entity_type);
    void clear();

    [[nodiscard]] GeometryEntityPtr findById(EntityId entity_id) const;
    [[nodiscard]] GeometryEntityPtr findByUIDAndType(EntityUID entity_uid,
                                                     EntityType entity_type) const;
    [[nodiscard]] GeometryEntityPtr findByShape(const TopoDS_Shape& shape) const;

    [[nodiscard]] size_t entityCount() const;
    [[nodiscard]] size_t entityCountByType(EntityType entity_type) const;

private:
    struct EntityTypeHash {
        std::size_t operator()(EntityType type) const noexcept {
            using Underlying = std::underlying_type_t<EntityType>;
            return std::hash<Underlying>{}(static_cast<Underlying>(type));
        }
    };

    struct TypeUIDHash {
        static void hashCombine(std::size_t& seed, std::size_t value) {
            seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        }

        std::size_t operator()(const std::pair<EntityType, EntityUID>& key) const noexcept {
            std::size_t seed = 0;
            hashCombine(seed, EntityTypeHash{}(key.first));
            hashCombine(seed, std::hash<EntityUID>()(key.second));
            return seed;
        }
    };

    struct IndexHandle {
        std::size_t m_slot{0};
        uint32_t m_generation{0};
    };

    struct Slot {
        GeometryEntityPtr m_entity;
        uint32_t m_generation{1};
    };

    std::vector<Slot> m_slots;
    std::vector<std::size_t> m_freeSlots;

    mutable std::unordered_map<EntityId, IndexHandle> m_byId;
    mutable std::unordered_map<std::pair<EntityType, EntityUID>, IndexHandle, TypeUIDHash>
        m_byTypeAndUID;
    mutable std::unordered_map<TopoDS_Shape, IndexHandle, TopTools_ShapeMapHasher> m_byShape;

    std::unordered_map<EntityType, std::size_t, EntityTypeHash> m_countByType;
    std::size_t m_aliveCount{0};
};
} // namespace OpenGeoLab::Geometry