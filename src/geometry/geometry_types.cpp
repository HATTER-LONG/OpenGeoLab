#include "geometry/geometry_types.hpp"
#include <atomic>

namespace OpenGeoLab::Geometry {

EntityId generateEntityId() {
    static std::atomic<EntityId> next_id{1};
    return next_id.fetch_add(1, std::memory_order_relaxed);
}

EntityUID generateEntityUID(EntityType type) { return INVALID_ENTITY_UID; }
} // namespace OpenGeoLab::Geometry