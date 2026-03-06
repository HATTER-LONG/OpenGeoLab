/**
 * @file geometry_documentImpl.cpp
 * @brief Implementation of GeometryDocument entity management
 */

#include "geometry_documentImpl.hpp"
#include "entity/geometry_entityImpl.hpp"
#include "geometry_render_builder.hpp"
#include "shape_builder.hpp"
#include "util/logger.hpp"
#include "util/progress_callback.hpp"

namespace OpenGeoLab::Geometry {

std::shared_ptr<GeometryDocumentImpl> GeometryDocumentImpl::instance() {
    static auto instance = std::make_shared<GeometryDocumentImpl>();
    return instance;
}

GeometryDocumentImplSingletonFactory::tObjectSharedPtr
GeometryDocumentImplSingletonFactory::instance() const {
    return GeometryDocumentImpl::instance();
}

GeometryDocumentImpl::GeometryDocumentImpl() : m_relationshipIndex(m_entityIndex) {}

bool GeometryDocumentImpl::addEntity(const GeometryEntityImplPtr& entity) {
    std::unique_lock<std::shared_mutex> lock(m_documentMutex);
    return addEntityUnlocked(entity);
}

bool GeometryDocumentImpl::addEntityUnlocked(const GeometryEntityImplPtr& entity) {
    if(!m_entityIndex.addEntity(entity)) {
        LOG_WARN("GeometryDocument: Failed to add entity id={}", entity ? entity->entityId() : 0);
        return false;
    }
    entity->setDocument(shared_from_this());
    LOG_TRACE("GeometryDocument: Added entity id={}, type={}", entity->entityId(),
              static_cast<int>(entity->entityType()));
    return true;
}

bool GeometryDocumentImpl::removeEntity(EntityId entity_id) {
    std::unique_lock<std::shared_mutex> lock(m_documentMutex);
    return removeEntityUnlocked(entity_id);
}

bool GeometryDocumentImpl::removeEntityUnlocked(EntityId entity_id) {
    const auto entity = findImplByIdUnlocked(entity_id);
    if(!entity) {
        LOG_DEBUG("GeometryDocument: Entity not found for removal, id={}", entity_id);
        return false;
    }

    if(!m_entityIndex.removeEntity(entity_id)) {
        LOG_WARN("GeometryDocument: Failed to remove entity id={}", entity_id);
        return false;
    }

    entity->setDocument({});
    LOG_TRACE("GeometryDocument: Removed entity id={}", entity_id);
    return true;
}

size_t GeometryDocumentImpl::removeEntityWithChildren(EntityId entity_id) {
    LOG_DEBUG("GeometryDocument: Removing entity and children, rootId={}", entity_id);
    size_t removed_count = 0;
    {
        std::unique_lock<std::shared_mutex> lock(m_documentMutex);
        removeEntityRecursive(entity_id, removed_count);
    }
    LOG_DEBUG("GeometryDocument: Removed {} entities", removed_count);
    return removed_count;
}

void GeometryDocumentImpl::removeEntityRecursive(EntityId entity_id, // NOLINT
                                                 size_t& removed_count) {
    const auto entity = m_entityIndex.findById(entity_id);
    if(!entity) {
        return;
    }

    // First, recursively remove all children
    const auto children_ids = m_relationshipIndex.directChildren(entity_id);
    for(const auto child_id : children_ids) {
        removeEntityRecursive(child_id, removed_count);
    }

    // Then remove this entity
    if(!m_entityIndex.removeEntity(entity_id)) {
        return;
    }

    entity->setDocument({});
    LOG_TRACE("GeometryDocument: Removed entity id={}", entity_id);
    {
        ++removed_count;
    }
}

void GeometryDocumentImpl::clear() {
    size_t count = 0;
    {
        std::unique_lock<std::shared_mutex> lock(m_documentMutex);
        count = entityCountUnlocked();
        clearUnlocked();
    }
    LOG_INFO("GeometryDocument: Cleared document, removed {} entities", count);
    emitChangeEvent(GeometryChangeEvent(GeometryChangeType::EntityRemoved, INVALID_ENTITY_ID));
}

void GeometryDocumentImpl::clearUnlocked() {
    m_relationshipIndex.clear();
    m_entityIndex.clear();
    resetEntityIdGenerator();
    resetAllEntityUIDGenerators();
}

GeometryEntityPtr GeometryDocumentImpl::findById(EntityId entity_id) const {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return findImplByIdUnlocked(entity_id);
}

GeometryEntityPtr GeometryDocumentImpl::findByUIDAndType(EntityUID entity_uid,
                                                         EntityType entity_type) const {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return findImplByUIDAndTypeUnlocked(entity_uid, entity_type);
}

GeometryEntityImplPtr GeometryDocumentImpl::findImplById(EntityId entity_id) const {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return findImplByIdUnlocked(entity_id);
}

GeometryEntityImplPtr GeometryDocumentImpl::findImplByIdUnlocked(EntityId entity_id) const {
    return m_entityIndex.findById(entity_id);
}

GeometryEntityImplPtr GeometryDocumentImpl::findImplByUIDAndType(EntityUID entity_uid,
                                                                 EntityType entity_type) const {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return findImplByUIDAndTypeUnlocked(entity_uid, entity_type);
}

GeometryEntityImplPtr
GeometryDocumentImpl::findImplByUIDAndTypeUnlocked(EntityUID entity_uid,
                                                   EntityType entity_type) const {
    return m_entityIndex.findByUIDAndType(entity_uid, entity_type);
}

GeometryEntityPtr GeometryDocumentImpl::findByShape(const TopoDS_Shape& shape) const {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return findByShapeUnlocked(shape);
}

GeometryEntityPtr GeometryDocumentImpl::findByShapeUnlocked(const TopoDS_Shape& shape) const {
    return m_entityIndex.findByShape(shape);
}

[[nodiscard]] size_t GeometryDocumentImpl::entityCount() const {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return entityCountUnlocked();
}

size_t GeometryDocumentImpl::entityCountUnlocked() const { return m_entityIndex.entityCount(); }

[[nodiscard]] size_t GeometryDocumentImpl::entityCountByType(EntityType entity_type) const {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return entityCountByTypeUnlocked(entity_type);
}

size_t GeometryDocumentImpl::entityCountByTypeUnlocked(EntityType entity_type) const {
    return m_entityIndex.entityCountByType(entity_type);
}

std::vector<GeometryEntityImplPtr>
GeometryDocumentImpl::entitiesByType(EntityType entity_type) const {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return entitiesByTypeUnlocked(entity_type);
}

std::vector<GeometryEntityImplPtr>
GeometryDocumentImpl::entitiesByTypeUnlocked(EntityType entity_type) const {
    return m_entityIndex.entitiesByType(entity_type);
}

std::vector<GeometryEntityImplPtr> GeometryDocumentImpl::allEntities() const {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return allEntitiesUnlocked();
}

std::vector<GeometryEntityImplPtr> GeometryDocumentImpl::allEntitiesUnlocked() const {
    return m_entityIndex.snapshotEntities();
}

bool GeometryDocumentImpl::addChildEdge(const GeometryEntityImpl& parent,
                                        const GeometryEntityImpl& child) {
    std::unique_lock<std::shared_mutex> lock(m_documentMutex);
    return addChildEdgeUnlocked(parent, child);
}

bool GeometryDocumentImpl::addChildEdgeUnlocked(const GeometryEntityImpl& parent,
                                                const GeometryEntityImpl& child) {
    if(parent.entityId() == child.entityId()) {
        return false;
    }

    // Enforce type-level relationship constraints.
    if(!parent.canAddChildType(child.entityType()) ||
       !child.canAddParentType(parent.entityType())) {
        return false;
    }

    return m_relationshipIndex.addRelationshipInfo(parent, child);
}

LoadResult GeometryDocumentImpl::loadFromShape(const TopoDS_Shape& shape,
                                               const std::string& name,
                                               Util::ProgressCallback progress) {
    LOG_INFO("GeometryDocument: Loading shape '{}' (replacing existing geometry)", name);

    if(shape.IsNull()) {
        LOG_ERROR("GeometryDocument: Input shape is null");
        return LoadResult::failure("Input shape is null");
    }

    if(!progress(0.0, "Clearing document...")) {
        LOG_DEBUG("GeometryDocument: Load cancelled during clear phase");
        return LoadResult::failure("Operation cancelled");
    }

    clear();

    if(!progress(0.1, "Starting shape load...")) {
        return LoadResult::failure("Operation cancelled");
    }

    auto subcallback = Util::makeScaledProgressCallback(progress, 0.1, 1.0);
    return appendShape(shape, name, subcallback);
}
LoadResult GeometryDocumentImpl::appendShape(const TopoDS_Shape& shape,
                                             const std::string& name,
                                             Util::ProgressCallback progress) {
    LOG_DEBUG("GeometryDocument: Appending shape '{}'", name);

    if(shape.IsNull()) {
        LOG_ERROR("GeometryDocument: Input shape is null");
        return LoadResult::failure("Input shape is null");
    }
    if(!progress(0.0, "Starting shape append...")) {
        LOG_DEBUG("GeometryDocument: Append cancelled");
        return LoadResult::failure("Operation cancelled");
    }
    try {
        ShapeBuilder builder(shared_from_this());

        auto subcallback = Util::makeScaledProgressCallback(progress, 0.1, 0.8);

        auto build_result = builder.buildFromShape(shape, name, subcallback);

        if(!build_result.m_success) {
            LOG_ERROR("GeometryDocument: Shape build failed: {}", build_result.m_errorMessage);
            return LoadResult::failure(build_result.m_errorMessage);
        }

        if(!progress(0.80, "Update geometry view data...")) {
            return LoadResult::failure("Operation cancelled");
        }

        // Invalidate render data and notify subscribers
        emitChangeEvent(GeometryChangeEvent(
            GeometryChangeType::EntityAdded,
            build_result.m_rootPart ? build_result.m_rootPart->entityId() : INVALID_ENTITY_ID));

        progress(1.0, "Load completed.");
        LOG_INFO("GeometryDocument: Shape '{}' loaded successfully, entityCount={}", name,
                 build_result.totalEntityCount());
        return LoadResult::success(build_result.m_rootPart->entityId(),
                                   build_result.totalEntityCount());
    } catch(const std::exception& e) {
        LOG_ERROR("GeometryDocument: Exception during shape load: {}", e.what());
        return LoadResult::failure(std::string("Exception: ") + e.what());
    }
}

std::vector<EntityKey> GeometryDocumentImpl::findRelatedEntities(EntityId entity_id,
                                                                 EntityType related_type) const {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return findRelatedEntitiesUnlocked(entity_id, related_type);
}

std::vector<EntityKey>
GeometryDocumentImpl::findRelatedEntitiesUnlocked(EntityId entity_id,
                                                  EntityType related_type) const {
    return m_relationshipIndex.findRelatedEntities(entity_id, related_type);
}

std::vector<EntityKey> GeometryDocumentImpl::findRelatedEntities(EntityUID entity_uid,
                                                                 EntityType entity_type,
                                                                 EntityType related_type) const {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return findRelatedEntitiesUnlocked(entity_uid, entity_type, related_type);
}

std::vector<EntityKey> GeometryDocumentImpl::findRelatedEntitiesUnlocked(
    EntityUID entity_uid, EntityType entity_type, EntityType related_type) const {
    return m_relationshipIndex.findRelatedEntities(entity_uid, entity_type, related_type);
}

// =============================================================================
// Render Data Implementation
// =============================================================================
bool GeometryDocumentImpl::getRenderData(Render::RenderData& render_data,
                                         const Render::TessellationOptions& options) {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return getRenderDataUnlocked(render_data, options);
}

bool GeometryDocumentImpl::getRenderDataUnlocked(Render::RenderData& render_data,
                                                 const Render::TessellationOptions& options) {
    GeometryRenderInput input{m_entityIndex, m_relationshipIndex, options};
    render_data.markGeometryUpdated();
    return GeometryRenderBuilder::build(render_data, input);
}

// =============================================================================
// Change Notification Implementation
// =============================================================================

Util::ScopedConnection
GeometryDocumentImpl::subscribeToChanges(std::function<void(const GeometryChangeEvent&)> callback) {
    return m_changeSignal.connect(std::move(callback));
}

void GeometryDocumentImpl::emitChangeEvent(const GeometryChangeEvent& event) {
    m_changeSignal.emitSignal(event);
}

} // namespace OpenGeoLab::Geometry
