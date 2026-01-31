/**
 * @file geometry_documentImpl.cpp
 * @brief Implementation of GeometryDocument entity management and operations
 */

#include "geometry_documentImpl.hpp"
#include "entity/geometry_entity.hpp"
#include "shape_builder.hpp"
#include "util/logger.hpp"

#include <BRepAdaptor_Curve.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_TangentialDeflection.hxx>
#include <Poly_Triangulation.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>

#include <queue>
#include <unordered_set>

namespace OpenGeoLab::Geometry {

bool GeometryDocumentImpl::addEntity(const GeometryEntityPtr& entity) {
    if(!m_entityIndex.addEntity(entity)) {
        return false;
    }
    entity->setDocument(shared_from_this());
    return true;
}

bool GeometryDocumentImpl::removeEntity(EntityId entity_id) {
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

size_t GeometryDocumentImpl::removeEntityWithChildren(EntityId entity_id) {
    size_t removed_count = 0;
    removeEntityRecursive(entity_id, removed_count);
    return removed_count;
}

void GeometryDocumentImpl::removeEntityRecursive(EntityId entity_id, // NOLINT
                                                 size_t& removed_count) {
    const auto entity = m_entityIndex.findById(entity_id);
    if(!entity) {
        return;
    }

    // First, recursively remove all children
    auto children = entity->children();
    for(const auto& child : children) {
        if(child) {
            removeEntityRecursive(child->entityId(), removed_count);
        }
    }

    // Then remove this entity
    if(removeEntity(entity_id)) {
        ++removed_count;
    }
}

void GeometryDocumentImpl::clear() {
    m_entityIndex.clear();
    invalidateRenderData();
    m_boundingBoxValid = false;
}

GeometryEntityPtr GeometryDocumentImpl::findById(EntityId entity_id) const {
    return m_entityIndex.findById(entity_id);
}

GeometryEntityPtr GeometryDocumentImpl::findByUIDAndType(EntityUID entity_uid,
                                                         EntityType entity_type) const {
    return m_entityIndex.findByUIDAndType(entity_uid, entity_type);
}

GeometryEntityPtr GeometryDocumentImpl::findByShape(const TopoDS_Shape& shape) const {
    return m_entityIndex.findByShape(shape);
}

std::vector<GeometryEntityPtr> GeometryDocumentImpl::entitiesByType(EntityType entity_type) const {
    return m_entityIndex.entitiesByType(entity_type);
}

std::vector<GeometryEntityPtr> GeometryDocumentImpl::allEntities() const {
    return m_entityIndex.snapshotEntities();
}

std::vector<GeometryEntityPtr> GeometryDocumentImpl::findAncestors(EntityId entity_id,
                                                                   EntityType ancestor_type) const {
    std::vector<GeometryEntityPtr> result;
    const auto entity = findById(entity_id);
    if(!entity) {
        return result;
    }

    // BFS traversal up the parent chain
    std::unordered_set<EntityId> visited;
    std::queue<EntityId> to_visit;

    for(const auto& parent : entity->parents()) {
        if(parent) {
            to_visit.push(parent->entityId());
        }
    }

    while(!to_visit.empty()) {
        EntityId current_id = to_visit.front();
        to_visit.pop();

        if(visited.count(current_id) > 0) {
            continue;
        }
        visited.insert(current_id);

        auto current = findById(current_id);
        if(!current) {
            continue;
        }

        if(current->entityType() == ancestor_type) {
            result.push_back(current);
        }

        // Continue searching through parents
        for(const auto& parent : current->parents()) {
            if(parent && visited.count(parent->entityId()) == 0) {
                to_visit.push(parent->entityId());
            }
        }
    }

    return result;
}

std::vector<GeometryEntityPtr>
GeometryDocumentImpl::findDescendants(EntityId entity_id, EntityType descendant_type) const {
    std::vector<GeometryEntityPtr> result;
    const auto entity = findById(entity_id);
    if(!entity) {
        return result;
    }

    // BFS traversal down the child tree
    std::unordered_set<EntityId> visited;
    std::queue<EntityId> to_visit;

    for(const auto& child : entity->children()) {
        if(child) {
            to_visit.push(child->entityId());
        }
    }

    while(!to_visit.empty()) {
        EntityId current_id = to_visit.front();
        to_visit.pop();

        if(visited.count(current_id) > 0) {
            continue;
        }
        visited.insert(current_id);

        auto current = findById(current_id);
        if(!current) {
            continue;
        }

        if(current->entityType() == descendant_type) {
            result.push_back(current);
        }

        // Continue searching through children
        for(const auto& child : current->children()) {
            if(child && visited.count(child->entityId()) == 0) {
                to_visit.push(child->entityId());
            }
        }
    }

    return result;
}

GeometryEntityPtr GeometryDocumentImpl::findOwningPart(EntityId entity_id) const {
    const auto entity = findById(entity_id);
    if(!entity) {
        return nullptr;
    }

    // If this is already a Part, return it
    if(entity->entityType() == EntityType::Part) {
        return entity;
    }

    // Traverse up the parent chain to find a Part
    std::unordered_set<EntityId> visited;
    std::queue<EntityId> to_visit;

    for(const auto& parent : entity->parents()) {
        if(parent) {
            to_visit.push(parent->entityId());
        }
    }

    while(!to_visit.empty()) {
        EntityId current_id = to_visit.front();
        to_visit.pop();

        if(visited.count(current_id) > 0) {
            continue;
        }
        visited.insert(current_id);

        auto current = findById(current_id);
        if(!current) {
            continue;
        }

        if(current->entityType() == EntityType::Part) {
            return current;
        }

        for(const auto& parent : current->parents()) {
            if(parent && visited.count(parent->entityId()) == 0) {
                to_visit.push(parent->entityId());
            }
        }
    }

    return nullptr;
}

std::vector<GeometryEntityPtr>
GeometryDocumentImpl::findRelatedEntities(EntityId edge_entity_id, EntityType related_type) const {
    std::vector<GeometryEntityPtr> result;

    const auto entity = findById(edge_entity_id);
    if(!entity) {
        return result;
    }

    // For edges, traverse up to find parent wires, then to faces
    if(entity->entityType() == EntityType::Edge && related_type == EntityType::Face) {
        // Edge -> Wire -> Face
        for(const auto& wire : entity->parents()) {
            if(!wire || wire->entityType() != EntityType::Wire) {
                continue;
            }
            for(const auto& face : wire->parents()) {
                if(face && face->entityType() == EntityType::Face) {
                    // Check for duplicates
                    bool found = false;
                    for(const auto& existing : result) {
                        if(existing->entityId() == face->entityId()) {
                            found = true;
                            break;
                        }
                    }
                    if(!found) {
                        result.push_back(face);
                    }
                }
            }
        }
    } else {
        // Generic approach: use findAncestors for other relationships
        result = findAncestors(edge_entity_id, related_type);
    }

    return result;
}

bool GeometryDocumentImpl::addChildEdge(EntityId parent_id, EntityId child_id) {
    if(parent_id == INVALID_ENTITY_ID || child_id == INVALID_ENTITY_ID) {
        return false;
    }
    if(parent_id == child_id) {
        return false;
    }

    const auto parent = findById(parent_id);
    const auto child = findById(child_id);
    if(!parent || !child) {
        return false;
    }

    // Enforce type-level relationship constraints.
    if(!parent->canAddChildType(child->entityType()) ||
       !child->canAddParentType(parent->entityType())) {
        return false;
    }

    const bool inserted = parent->addChildNoSync(child_id);
    if(!inserted) {
        return false;
    }

    (void)child->addParentNoSync(parent_id);
    return true;
}

bool GeometryDocumentImpl::removeChildEdge(EntityId parent_id, EntityId child_id) {
    if(parent_id == INVALID_ENTITY_ID || child_id == INVALID_ENTITY_ID) {
        return false;
    }
    if(parent_id == child_id) {
        return false;
    }

    const auto parent = findById(parent_id);
    if(!parent) {
        return false;
    }

    const bool erased = parent->removeChildNoSync(child_id);
    if(!erased) {
        return false;
    }

    // Best-effort: if child already expired, local removal is enough.
    if(const auto child = findById(child_id)) {
        (void)child->removeParentNoSync(parent_id);
    }

    return true;
}

// =============================================================================
// GeometryDocument Interface Implementation
// =============================================================================

LoadResult GeometryDocumentImpl::loadFromShape(const TopoDS_Shape& shape,
                                               const std::string& name,
                                               DocumentProgressCallback progress) {
    if(shape.IsNull()) {
        return LoadResult::failure("Input shape is null");
    }

    if(!progress(0.0, "Starting shape load...")) {
        return LoadResult::failure("Operation cancelled");
    }

    try {
        ShapeBuilder builder(shared_from_this());

        auto scaledProgress = [&progress](double p, const std::string& msg) {
            return progress(p * 0.9, msg);
        };

        auto buildResult = builder.buildFromShape(shape, name, scaledProgress);

        if(!buildResult.m_success) {
            return LoadResult::failure(buildResult.m_errorMessage);
        }

        if(!progress(0.95, "Finalizing...")) {
            return LoadResult::failure("Operation cancelled");
        }

        invalidateRenderData();
        m_boundingBoxValid = false;

        // Notify observers of new shape
        GeometryChangeEvent event;
        event.m_type = GeometryChangeType::ShapeLoaded;
        event.m_affectedEntities.push_back(buildResult.m_rootPart->entityId());
        event.m_message = "Shape loaded: " + name;
        notifyGeometryChanged(event);

        progress(1.0, "Load completed.");

        return LoadResult::success(buildResult.m_rootPart->entityId(),
                                   buildResult.totalEntityCount());

    } catch(const std::exception& e) {
        LOG_ERROR("Exception during shape load: {}", e.what());
        return LoadResult::failure(std::string("Exception: ") + e.what());
    }
}

bool GeometryDocumentImpl::buildShapes() { return true; }

bool GeometryDocumentImpl::deleteEntities(const std::vector<EntityId>& entityIds,
                                          bool deleteChildren) {
    std::vector<EntityId> deletedIds;

    for(EntityId id : entityIds) {
        if(deleteChildren) {
            size_t removed = removeEntityWithChildren(id);
            if(removed > 0) {
                deletedIds.push_back(id);
            }
        } else {
            if(removeEntity(id)) {
                deletedIds.push_back(id);
            }
        }
    }

    if(!deletedIds.empty()) {
        invalidateRenderData();
        m_boundingBoxValid = false;

        // Notify observers of deletion
        GeometryChangeEvent event;
        event.m_type = GeometryChangeType::ShapeDeleted;
        event.m_affectedEntities = deletedIds;
        event.m_message = "Entities deleted";
        notifyGeometryChanged(event);
    }

    return !deletedIds.empty();
}

// =============================================================================
// Observer Pattern Implementation
// =============================================================================

void GeometryDocumentImpl::addChangeObserver(IGeometryChangeObserver* observer) {
    if(observer == nullptr) {
        return;
    }
    auto it = std::find(m_changeObservers.begin(), m_changeObservers.end(), observer);
    if(it == m_changeObservers.end()) {
        m_changeObservers.push_back(observer);
    }
}

void GeometryDocumentImpl::removeChangeObserver(IGeometryChangeObserver* observer) {
    auto it = std::find(m_changeObservers.begin(), m_changeObservers.end(), observer);
    if(it != m_changeObservers.end()) {
        m_changeObservers.erase(it);
    }
}

void GeometryDocumentImpl::notifyGeometryChanged(const GeometryChangeEvent& event) {
    for(auto* observer : m_changeObservers) {
        if(observer != nullptr) {
            observer->onGeometryChanged(event);
        }
    }
}

const RenderContext* GeometryDocumentImpl::getRenderContext() const {
    if(!m_renderDataValid) {
        (void)const_cast<GeometryDocumentImpl*>(this)->rebuildRenderData();
    }
    return &m_renderContext;
}

bool GeometryDocumentImpl::rebuildRenderData(DocumentProgressCallback progress) {
    if(!progress(0.0, "Rebuilding render data...")) {
        return false;
    }

    m_renderContext.clear();

    auto faces = entitiesByType(EntityType::Face);
    auto edges = entitiesByType(EntityType::Edge);
    auto vertices = entitiesByType(EntityType::Vertex);

    size_t totalItems = faces.size() + edges.size() + vertices.size();
    size_t processedItems = 0;

    for(const auto& faceEntity : faces) {
        auto mesh = buildMeshForFace(faceEntity);
        if(mesh.isValid()) {
            m_renderContext.m_meshes.push_back(std::move(mesh));
        }
        ++processedItems;
        if(totalItems > 0 && !progress(static_cast<double>(processedItems) / totalItems * 0.9,
                                       "Processing faces...")) {
            return false;
        }
    }

    for(const auto& edgeEntity : edges) {
        auto edge = buildEdgeForEdge(edgeEntity);
        if(edge.isValid()) {
            m_renderContext.m_edges.push_back(std::move(edge));
        }
        ++processedItems;
    }

    for(const auto& vertexEntity : vertices) {
        auto vertex = buildVertexForVertex(vertexEntity);
        m_renderContext.m_vertices.push_back(vertex);
        ++processedItems;
    }

    m_renderContext.m_boundingBox = boundingBox();
    m_renderDataValid = true;
    progress(1.0, "Render data rebuild completed.");
    return true;
}

RenderMesh GeometryDocumentImpl::getRenderMeshForFace(EntityId faceEntityId) const {
    auto entity = findById(faceEntityId);
    if(!entity || entity->entityType() != EntityType::Face) {
        return RenderMesh();
    }
    return buildMeshForFace(entity);
}

RenderEdge GeometryDocumentImpl::getRenderEdgeForEdge(EntityId edgeEntityId) const {
    auto entity = findById(edgeEntityId);
    if(!entity || entity->entityType() != EntityType::Edge) {
        return RenderEdge();
    }
    return buildEdgeForEdge(entity);
}

RenderMesh GeometryDocumentImpl::buildMeshForFace(const GeometryEntityPtr& faceEntity) const {
    RenderMesh mesh;
    mesh.m_entityId = faceEntity->entityId();

    const TopoDS_Shape& shape = faceEntity->shape();
    if(shape.IsNull() || shape.ShapeType() != TopAbs_FACE) {
        return mesh;
    }

    TopoDS_Face face = TopoDS::Face(shape);

    BRepMesh_IncrementalMesh mesher(static_cast<TopoDS_Shape>(face), 0.1);
    mesher.Perform();

    TopLoc_Location location;
    Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);

    if(triangulation.IsNull()) {
        return mesh;
    }

    int numNodes = triangulation->NbNodes();
    mesh.m_vertices.reserve(numNodes * 3);
    mesh.m_normals.reserve(numNodes * 3);

    for(int i = 1; i <= numNodes; ++i) {
        gp_Pnt point = triangulation->Node(i).Transformed(location);
        mesh.m_vertices.push_back(static_cast<float>(point.X()));
        mesh.m_vertices.push_back(static_cast<float>(point.Y()));
        mesh.m_vertices.push_back(static_cast<float>(point.Z()));

        if(triangulation->HasNormals()) {
            gp_Vec normalVec = triangulation->Normal(i);
            if(normalVec.Magnitude() > 0.0) {
                gp_Dir normal(normalVec);
                mesh.m_normals.push_back(static_cast<float>(normal.X()));
                mesh.m_normals.push_back(static_cast<float>(normal.Y()));
                mesh.m_normals.push_back(static_cast<float>(normal.Z()));
            } else {
                mesh.m_normals.push_back(0.0f);
                mesh.m_normals.push_back(0.0f);
                mesh.m_normals.push_back(1.0f);
            }
        } else {
            mesh.m_normals.push_back(0.0f);
            mesh.m_normals.push_back(0.0f);
            mesh.m_normals.push_back(1.0f);
        }
    }

    int numTriangles = triangulation->NbTriangles();
    mesh.m_indices.reserve(numTriangles * 3);

    for(int i = 1; i <= numTriangles; ++i) {
        const Poly_Triangle& tri = triangulation->Triangle(i);
        int n1, n2, n3;
        tri.Get(n1, n2, n3);

        mesh.m_indices.push_back(static_cast<uint32_t>(n1 - 1));
        mesh.m_indices.push_back(static_cast<uint32_t>(n2 - 1));
        mesh.m_indices.push_back(static_cast<uint32_t>(n3 - 1));
    }

    return mesh;
}

RenderEdge GeometryDocumentImpl::buildEdgeForEdge(const GeometryEntityPtr& edgeEntity) const {
    RenderEdge edge;
    edge.m_entityId = edgeEntity->entityId();

    const TopoDS_Shape& shape = edgeEntity->shape();
    if(shape.IsNull() || shape.ShapeType() != TopAbs_EDGE) {
        return edge;
    }

    const TopoDS_Edge& topoEdge = TopoDS::Edge(shape);

    try {
        BRepAdaptor_Curve curve(topoEdge);
        GCPnts_TangentialDeflection discretizer(curve, 0.1, 0.1);

        int numPoints = discretizer.NbPoints();
        edge.m_points.reserve(numPoints * 3);

        for(int i = 1; i <= numPoints; ++i) {
            gp_Pnt point = discretizer.Value(i);
            edge.m_points.push_back(static_cast<float>(point.X()));
            edge.m_points.push_back(static_cast<float>(point.Y()));
            edge.m_points.push_back(static_cast<float>(point.Z()));
        }
    } catch(const Standard_Failure& e) {
        LOG_WARN("Failed to discretize edge: {}",
                 e.GetMessageString() ? e.GetMessageString() : "Unknown");
    }

    return edge;
}

RenderVertex
GeometryDocumentImpl::buildVertexForVertex(const GeometryEntityPtr& vertexEntity) const {
    RenderVertex vertex;
    vertex.m_entityId = vertexEntity->entityId();

    const TopoDS_Shape& shape = vertexEntity->shape();
    if(shape.IsNull() || shape.ShapeType() != TopAbs_VERTEX) {
        return vertex;
    }

    gp_Pnt point = BRep_Tool::Pnt(TopoDS::Vertex(shape));
    vertex.m_x = static_cast<float>(point.X());
    vertex.m_y = static_cast<float>(point.Y());
    vertex.m_z = static_cast<float>(point.Z());

    return vertex;
}

void GeometryDocumentImpl::invalidateRenderData() { m_renderDataValid = false; }

size_t GeometryDocumentImpl::entityCount() const { return m_entityIndex.entityCount(); }

size_t GeometryDocumentImpl::entityCountByType(EntityType type) const {
    return m_entityIndex.entityCountByType(type);
}

BoundingBox3D GeometryDocumentImpl::boundingBox() const {
    if(!m_boundingBoxValid) {
        m_boundingBox = BoundingBox3D();

        auto allEnts = m_entityIndex.snapshotEntities();
        for(const auto& entity : allEnts) {
            if(entity->hasBoundingBox()) {
                m_boundingBox.expand(entity->boundingBox());
            }
        }

        m_boundingBoxValid = true;
    }
    return m_boundingBox;
}

bool GeometryDocumentImpl::isEmpty() const { return m_entityIndex.entityCount() == 0; }

EntityId
GeometryDocumentImpl::pickEntity(int /*screenX*/, int /*screenY*/, SelectionMode /*mode*/) const {
    LOG_WARN("pickEntity not yet implemented");
    return INVALID_ENTITY_ID;
}

std::vector<EntityId> GeometryDocumentImpl::pickEntitiesInRect(
    int /*x1*/, int /*y1*/, int /*x2*/, int /*y2*/, SelectionMode /*mode*/) const {
    LOG_WARN("pickEntitiesInRect not yet implemented");
    return {};
}

} // namespace OpenGeoLab::Geometry
