#pragma once

#include <kangaroo/util/noncopyable.hpp>

#include "geometry/entity_index.hpp"

#include <memory>

namespace OpenGeoLab::Geometry {

/**
 * @brief Geometry document holding the authoritative entity index.
 *
 * The document is the only owner and user of EntityIndex. Entities keep a
 * weak back-reference to the document for relationship resolution.
 */
class GeometryDocument : public Kangaroo::Util::NonCopyMoveable,
                         public std::enable_shared_from_this<GeometryDocument> {
public:
    using Ptr = std::shared_ptr<GeometryDocument>;

    [[nodiscard]] static Ptr create() {
        struct MakeSharedEnabler final : public GeometryDocument {
            MakeSharedEnabler() = default;
        };
        return std::make_shared<MakeSharedEnabler>();
    }

    ~GeometryDocument() = default;

    [[nodiscard]] bool addEntity(const GeometryEntityPtr& entity) {
        if(!m_entityIndex.addEntity(entity)) {
            return false;
        }
        entity->setDocument(shared_from_this());
        return true;
    }

    [[nodiscard]] bool removeEntity(EntityId entity_id) {
        const auto entity = m_entityIndex.findById(entity_id);
        if(!entity) {
            return false;
        }

        if(!m_entityIndex.removeEntity(entity_id)) {
            return false;
        }

        entity->setDocument({});
        return true;
    }

    void clear() {
        // Best-effort detach document back-references.
        for(const auto& entity : m_entityIndex.snapshotEntities()) {
            if(entity) {
                entity->setDocument({});
            }
        }
        m_entityIndex.clear();
    }

    [[nodiscard]] GeometryEntityPtr findById(EntityId entity_id) const {
        return m_entityIndex.findById(entity_id);
    }

    [[nodiscard]] GeometryEntityPtr findByUIDAndType(EntityUID entity_uid,
                                                     EntityType entity_type) const {
        return m_entityIndex.findByUIDAndType(entity_uid, entity_type);
    }

    [[nodiscard]] GeometryEntityPtr findByShape(const TopoDS_Shape& shape) const {
        return m_entityIndex.findByShape(shape);
    }

    [[nodiscard]] size_t entityCount() const { return m_entityIndex.entityCount(); }
    [[nodiscard]] size_t entityCountByType(EntityType entity_type) const {
        return m_entityIndex.entityCountByType(entity_type);
    }

    [[nodiscard]] bool addChildEdge(EntityId parent_id, EntityId child_id);
    [[nodiscard]] bool removeChildEdge(EntityId parent_id, EntityId child_id);

private:
    GeometryDocument() = default;

private:
    EntityIndex m_entityIndex;
};

} // namespace OpenGeoLab::Geometry