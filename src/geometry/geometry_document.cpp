/**
 * @file geometry_document.cpp
 * @brief Implementation of GeometryDocument, EntityIndex, and GeometryManager
 */

#include "geometry/geometry_document.hpp"

#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Iterator.hxx>

#include <algorithm>

namespace OpenGeoLab::Geometry {

// =============================================================================
// EntityIndex Implementation
// =============================================================================

void EntityIndex::addEntity(const GeometryEntityPtr& entity) {
    if(!entity) {
        return;
    }

    // Add to ID index
    m_byId[entity->entityId()] = entity;

    // Add to type+UID index
    m_byTypeAndUID[{entity->entityType(), entity->entityUID()}] = entity;

    // Add to shape hash index (if entity has a shape)
    if(entity->hasShape()) {
        const size_t shape_hash = std::hash<const void*>()(entity->shape().TShape().get());
        m_byShapeHash[shape_hash] = entity;
    }

    // Add to per-type list
    m_byType[entity->entityType()].push_back(entity);
}

bool EntityIndex::removeEntity(const GeometryEntityPtr& entity) {
    if(!entity) {
        return false;
    }

    // Check if entity exists
    auto it = m_byId.find(entity->entityId());
    if(it == m_byId.end()) {
        return false;
    }

    // Remove from ID index
    m_byId.erase(it);

    // Remove from type+UID index
    m_byTypeAndUID.erase({entity->entityType(), entity->entityUID()});

    // Remove from shape hash index
    if(entity->hasShape()) {
        const size_t shape_hash = std::hash<const void*>()(entity->shape().TShape().get());
        m_byShapeHash.erase(shape_hash);
    }

    // Remove from per-type list
    auto& type_list = m_byType[entity->entityType()];
    type_list.erase(std::remove(type_list.begin(), type_list.end(), entity), type_list.end());

    return true;
}

void EntityIndex::clear() {
    m_byId.clear();
    m_byTypeAndUID.clear();
    m_byShapeHash.clear();
    m_byType.clear();
}

GeometryEntityPtr EntityIndex::findById(EntityId id) const {
    auto it = m_byId.find(id);
    return (it != m_byId.end()) ? it->second : nullptr;
}

GeometryEntityPtr EntityIndex::findByTypeAndUID(EntityType type, EntityUID uid) const {
    auto it = m_byTypeAndUID.find({type, uid});
    return (it != m_byTypeAndUID.end()) ? it->second : nullptr;
}

GeometryEntityPtr EntityIndex::findByShape(const TopoDS_Shape& shape) const {
    if(shape.IsNull()) {
        return nullptr;
    }

    const size_t shape_hash = std::hash<const void*>()(shape.TShape().get());
    auto it = m_byShapeHash.find(shape_hash);
    return (it != m_byShapeHash.end()) ? it->second : nullptr;
}

std::vector<GeometryEntityPtr> EntityIndex::getEntitiesByType(EntityType type) const {
    auto it = m_byType.find(type);
    return (it != m_byType.end()) ? it->second : std::vector<GeometryEntityPtr>{};
}

size_t EntityIndex::entityCountByType(EntityType type) const {
    auto it = m_byType.find(type);
    return (it != m_byType.end()) ? it->second.size() : 0;
}

// =============================================================================
// GeometryDocument Implementation
// =============================================================================

GeometryDocument::GeometryDocument() = default;

GeometryDocument::~GeometryDocument() { clear(); }

void GeometryDocument::registerEntity(const GeometryEntityPtr& entity) {
    if(!entity) {
        return;
    }

    m_index.addEntity(entity);

    // Track root entities (those without parents)
    if(entity->parent().expired()) {
        m_rootEntities.push_back(entity);
    }

    m_modified = true;
}

bool GeometryDocument::unregisterEntity(const GeometryEntityPtr& entity) {
    if(!entity) {
        return false;
    }

    // Recursively unregister children first
    unregisterEntityRecursive(entity);

    // Remove from parent if any
    if(auto parent = entity->parent().lock()) {
        parent->removeChild(entity);
    }

    // Remove from root entities list
    m_rootEntities.erase(std::remove(m_rootEntities.begin(), m_rootEntities.end(), entity),
                         m_rootEntities.end());

    m_modified = true;
    return true;
}

void GeometryDocument::unregisterEntityRecursive(const GeometryEntityPtr& entity) { // NOLINT
    if(!entity) {
        return;
    }

    // Recursively unregister all children
    for(const auto& child : entity->children()) {
        unregisterEntityRecursive(child);
    }

    // Remove from index
    m_index.removeEntity(entity);
}

void GeometryDocument::clear() {
    m_rootEntities.clear();
    m_index.clear();
    m_modified = false;
}

GeometryEntityPtr GeometryDocument::buildTopologyHierarchy(const TopoDS_Shape& shape,
                                                           const std::string& name) {
    if(shape.IsNull()) {
        return nullptr;
    }

    // Create root entity using factory
    auto root = createEntityFromShape(shape);
    if(!root) {
        return nullptr;
    }
    root->setName(name.empty() ? "Part" : name);

    // Register root
    registerEntity(root);

    // Build sub-entities recursively
    buildSubEntities(root);

    return root;
}

void GeometryDocument::buildSubEntities(const GeometryEntityPtr& entity) { // NOLINT
    if(!entity || !entity->hasShape()) {
        return;
    }

    const TopoDS_Shape& shape = entity->shape();
    TopAbs_ShapeEnum shape_type = shape.ShapeType();

    // Determine what sub-shapes to explore based on current shape type
    TopAbs_ShapeEnum sub_type = TopAbs_SHAPE;

    switch(shape_type) {
    case TopAbs_COMPOUND:
    case TopAbs_COMPSOLID:
        sub_type = TopAbs_SOLID;
        break;
    case TopAbs_SOLID:
        sub_type = TopAbs_SHELL;
        break;
    case TopAbs_SHELL:
        sub_type = TopAbs_FACE;
        break;
    case TopAbs_FACE:
        sub_type = TopAbs_WIRE;
        break;
    case TopAbs_WIRE:
        sub_type = TopAbs_EDGE;
        break;
    case TopAbs_EDGE:
        sub_type = TopAbs_VERTEX;
        break;
    default:
        return; // No sub-shapes for vertices or unknown types
    }

    // Get sub-shapes
    std::vector<TopoDS_Shape> sub_shapes = getSubShapes(shape, sub_type);

    // Create child entities for each sub-shape
    for(const auto& sub_shape : sub_shapes) {
        // Avoid duplicates: check if we already have this shape
        if(m_index.findByShape(sub_shape)) {
            continue;
        }

        auto child = buildEntityFromShape(sub_shape, entity);
        if(child) {
            entity->addChild(child);
            registerEntity(child);

            // Recursively build children
            buildSubEntities(child);
        }
    }
}

GeometryEntityPtr GeometryDocument::buildEntityFromShape(const TopoDS_Shape& shape,
                                                         const GeometryEntityPtr& parent) {
    if(shape.IsNull()) {
        return nullptr;
    }

    // Use factory function to create appropriate entity type
    auto entity = createEntityFromShape(shape);
    if(!entity) {
        return nullptr;
    }

    // Generate default name based on type and UID
    entity->setName(std::string(entity->typeName()) + "_" + std::to_string(entity->entityUID()));

    if(parent) {
        entity->setParent(parent);
    }

    return entity;
}

std::vector<TopoDS_Shape> GeometryDocument::getSubShapes(const TopoDS_Shape& shape,
                                                         TopAbs_ShapeEnum sub_type) const {
    std::vector<TopoDS_Shape> result;

    if(shape.IsNull()) {
        return result;
    }

    // Use TopExp_Explorer to find all sub-shapes of the given type
    for(TopExp_Explorer explorer(shape, sub_type); explorer.More(); explorer.Next()) {
        result.push_back(explorer.Current());
    }

    return result;
}

// =============================================================================
// GeometryManager Implementation
// =============================================================================

GeometryManager& GeometryManager::instance() {
    static GeometryManager s_instance;
    return s_instance;
}

GeometryEntityPtr GeometryManager::findById(EntityId id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_globalIndex.findById(id);
}

GeometryEntityPtr GeometryManager::findByTypeAndUID(EntityType type, EntityUID uid) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_globalIndex.findByTypeAndUID(type, uid);
}

GeometryEntityPtr GeometryManager::findByShape(const TopoDS_Shape& shape) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_globalIndex.findByShape(shape);
}

std::vector<GeometryEntityPtr> GeometryManager::getEntitiesByType(EntityType type) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_globalIndex.getEntitiesByType(type);
}

size_t GeometryManager::entityCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_globalIndex.entityCount();
}

size_t GeometryManager::entityCountByType(EntityType type) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_globalIndex.entityCountByType(type);
}

void GeometryManager::registerEntity(const GeometryEntityPtr& entity) {
    if(!entity) {
        return;
    }
    m_globalIndex.addEntity(entity);
}

bool GeometryManager::unregisterEntity(const GeometryEntityPtr& entity) {
    if(!entity) {
        return false;
    }
    return m_globalIndex.removeEntity(entity);
}

void GeometryManager::clear() {
    m_globalIndex.clear();
    m_activeDocument.reset();
    m_documents.clear();
}

GeometryDocumentPtr GeometryManager::createDocument(const std::string& name) {
    auto doc = std::make_shared<GeometryDocument>();
    doc->setName(name);
    m_documents.push_back(doc);
    m_activeDocument = doc;
    return doc;
}

GeometryEntityPtr GeometryManager::buildTopologyHierarchy(const TopoDS_Shape& shape,
                                                          const std::string& name) {
    // Ensure we have an active document
    if(!m_activeDocument) {
        createDocument("Default");
    }

    // Build topology in document
    auto root = m_activeDocument->buildTopologyHierarchy(shape, name);

    // Register all entities with global index
    if(root) {
        registerEntityRecursive(root);
    }

    return root;
}

void GeometryManager::registerEntityRecursive(const GeometryEntityPtr& entity) { // NOLINT
    if(!entity) {
        return;
    }

    m_globalIndex.addEntity(entity);

    for(const auto& child : entity->children()) {
        registerEntityRecursive(child);
    }
}

void GeometryManager::unregisterEntityRecursive(const GeometryEntityPtr& entity) { // NOLINT
    if(!entity) {
        return;
    }

    for(const auto& child : entity->children()) {
        unregisterEntityRecursive(child);
    }

    m_globalIndex.removeEntity(entity);
}

} // namespace OpenGeoLab::Geometry
