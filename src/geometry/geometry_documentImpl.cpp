/**
 * @file geometry_documentImpl.cpp
 * @brief Implementation of GeometryDocument entity management
 */

#include "geometry_documentImpl.hpp"
#include "entity/geometry_entityImpl.hpp"
#include "shape_builder.hpp"
#include "util/logger.hpp"
#include "util/progress_callback.hpp"

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_UniformDeflection.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Poly_Polygon3D.hxx>
#include <Poly_Triangle.hxx>
#include <Poly_Triangulation.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>

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

namespace {
/**
 * @brief Check if transformation is identity
 * @param trsf Transformation to check
 * @return true if transformation is identity
 */
bool isIdentityTrsf(const gp_Trsf& trsf) {
    return trsf.IsNegative() == Standard_False && trsf.ScaleFactor() == 1.0 &&
           trsf.TranslationPart().SquareModulus() == 0.0;
}

} // namespace

bool GeometryDocumentImpl::addEntity(const GeometryEntityImplPtr& entity) {
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
    const auto entity = m_entityIndex.findById(entity_id);
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
    removeEntityRecursive(entity_id, removed_count);
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
    if(removeEntity(entity_id)) {
        ++removed_count;
    }
}

void GeometryDocumentImpl::clear() {
    const size_t count = m_entityIndex.entityCount();
    m_relationshipIndex.clear();
    m_entityIndex.clear();
    resetEntityIdGenerator();
    resetAllEntityUIDGenerators();
    LOG_INFO("GeometryDocument: Cleared document, removed {} entities", count);
    emitChangeEvent(GeometryChangeEvent(GeometryChangeType::EntityRemoved, INVALID_ENTITY_ID));
}

GeometryEntityPtr GeometryDocumentImpl::findById(EntityId entity_id) const {
    return m_entityIndex.findById(entity_id);
}

GeometryEntityPtr GeometryDocumentImpl::findByUIDAndType(EntityUID entity_uid,
                                                         EntityType entity_type) const {
    return m_entityIndex.findByUIDAndType(entity_uid, entity_type);
}

GeometryEntityImplPtr GeometryDocumentImpl::findImplById(EntityId entity_id) const {
    return m_entityIndex.findById(entity_id);
}

GeometryEntityImplPtr GeometryDocumentImpl::findImplByUIDAndType(EntityUID entity_uid,
                                                                 EntityType entity_type) const {
    return m_entityIndex.findByUIDAndType(entity_uid, entity_type);
}

GeometryEntityPtr GeometryDocumentImpl::findByShape(const TopoDS_Shape& shape) const {
    return m_entityIndex.findByShape(shape);
}

[[nodiscard]] size_t GeometryDocumentImpl::entityCount() const {
    return m_entityIndex.entityCount();
}

[[nodiscard]] size_t GeometryDocumentImpl::entityCountByType(EntityType entity_type) const {
    return m_entityIndex.entityCountByType(entity_type);
}

std::vector<GeometryEntityImplPtr>
GeometryDocumentImpl::entitiesByType(EntityType entity_type) const {
    return m_entityIndex.entitiesByType(entity_type);
}

std::vector<GeometryEntityImplPtr> GeometryDocumentImpl::allEntities() const {
    return m_entityIndex.snapshotEntities();
}

bool GeometryDocumentImpl::addChildEdge(const GeometryEntityImpl& parent,
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
    return m_relationshipIndex.findRelatedEntities(entity_id, related_type);
}

std::vector<EntityKey> GeometryDocumentImpl::findRelatedEntities(EntityUID entity_uid,
                                                                 EntityType entity_type,
                                                                 EntityType related_type) const {
    return m_relationshipIndex.findRelatedEntities(entity_uid, entity_type, related_type);
}

// =============================================================================
// Render Data Implementation
// =============================================================================

bool GeometryDocumentImpl::getRenderData(Render::RenderData& render_data,
                                         const Render::TessellationOptions& options) {
    std::lock_guard<std::mutex> lock(m_renderDataMutex);
    render_data.m_geometry.clear();

    auto faces = m_entityIndex.entitiesByType(EntityType::Face);
    auto edges = m_entityIndex.entitiesByType(EntityType::Edge);
    auto vertices = m_entityIndex.entitiesByType(EntityType::Vertex);

    render_data.m_geometry.reserve(faces.size() + edges.size() + vertices.size());

    for(const auto& face : faces) {
        generateFaceMesh(render_data, face, options);
    }
    for(const auto& edge : edges) {
        generateEdgeMesh(render_data, edge, options);
    }
    for(const auto& vertex : vertices) {
        generateVertexMesh(render_data, vertex, options);
    }
    return true;
}

void GeometryDocumentImpl::generateFaceMesh(Render::RenderData& render_data,
                                            const GeometryEntityImplPtr& entity,
                                            const Render::TessellationOptions& options) {
    if(!entity || !entity->hasShape()) {
        return;
    }

    const auto face = TopoDS::Face(entity->shape());

    BRepMesh_IncrementalMesh mesher(face, options.m_linearDeflection, Standard_False,
                                    options.m_angularDeflection, Standard_False);
    (void)mesher;
    TopLoc_Location loc;
    const Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, loc);
    if(triangulation.IsNull() || triangulation->NbNodes() < 3 || triangulation->NbTriangles() < 1) {
        return;
    }

    Render::RenderPrimitive face_primitive;
    face_primitive.m_uid = entity->entityUID();
    face_primitive.m_entityType = Render::RenderEntityType::Face;
    face_primitive.m_topology = Render::PrimitiveTopology::Triangles;
    face_primitive.m_passType = Render::RenderPassType::Geometry;
    auto part_key = findRelatedEntities(entity->entityId(), EntityType::Part);

    if(!part_key.empty()) {
        face_primitive.m_partUID = part_key.front().m_uid;
    }
    const int reserved_nodes = triangulation->NbNodes();
    const int triangles = triangulation->NbTriangles();
    face_primitive.m_positions.reserve(reserved_nodes);
    face_primitive.m_indices.reserve(triangles * 3);

    const gp_Trsf& trsf = loc.Transformation();
    const bool has_non_identity_trsf = !isIdentityTrsf(trsf);
    for(int i = 1; i <= triangulation->NbNodes(); ++i) {
        const gp_Pnt& pnt = triangulation->Node(i);
        if(has_non_identity_trsf) {
            gp_Pnt transformed_pnt = pnt.Transformed(trsf);
            face_primitive.m_positions.emplace_back(transformed_pnt.X(), transformed_pnt.Y(),
                                                    transformed_pnt.Z());
        } else {
            face_primitive.m_positions.emplace_back(pnt.X(), pnt.Y(), pnt.Z());
        }
    }

    for(int i = 1; i <= triangles; ++i) {
        const Poly_Triangle& tri = triangulation->Triangle(i);
        Standard_Integer n1 = 0, n2 = 0, n3 = 0;
        tri.Get(n1, n2, n3);
        if(face.Orientation() == TopAbs_REVERSED) {
            std::swap(n1, n3);
        }
        face_primitive.m_indices.push_back(static_cast<uint32_t>(n1 - 1));
        face_primitive.m_indices.push_back(static_cast<uint32_t>(n2 - 1));
        face_primitive.m_indices.push_back(static_cast<uint32_t>(n3 - 1));
    }

    if(face_primitive.isValid()) {
        render_data.m_geometry.push_back(std::move(face_primitive));
    }
    return;
}

void GeometryDocumentImpl::generateEdgeMesh(Render::RenderData& render_data,
                                            const GeometryEntityImplPtr& entity,
                                            const Render::TessellationOptions& options) {
    if(!entity || !entity->hasShape()) {
        return;
    }
    auto edge = TopoDS::Edge(entity->shape());

    Render::RenderPrimitive edge_primitive;
    edge_primitive.m_uid = entity->entityUID();
    edge_primitive.m_entityType = Render::RenderEntityType::Edge;
    edge_primitive.m_topology = Render::PrimitiveTopology::Lines;
    edge_primitive.m_passType = Render::RenderPassType::Geometry;
    auto part_key = findRelatedEntities(entity->entityId(), EntityType::Part);
    if(!part_key.empty()) {
        edge_primitive.m_partUID = part_key.front().m_uid;
    }

    TopLoc_Location location;
    const Handle(Poly_Polygon3D) polygon = BRep_Tool::Polygon3D(edge, location);
    const gp_Trsf trsf = location.Transformation();
    if(!polygon.IsNull() && polygon->NbNodes() >= 2) {
        const auto& nodes = polygon->Nodes();
        const Standard_Integer lower = nodes.Lower();
        const Standard_Integer upper = nodes.Upper();

        const Standard_Integer count = upper - lower + 1;
        edge_primitive.m_positions.reserve(static_cast<size_t>(count));
        edge_primitive.m_indices.reserve(
            static_cast<size_t>(std::max<Standard_Integer>(0, count - 1)) * 2);

        for(Standard_Integer idx = lower; idx <= upper; ++idx) {
            gp_Pnt point = nodes.Value(idx);
            if(!isIdentityTrsf(trsf)) {
                point.Transform(trsf);
            }
            edge_primitive.m_positions.emplace_back(point.X(), point.Y(), point.Z());
        }

        for(Standard_Integer i = 0; i + 1 < count; ++i) {
            edge_primitive.m_indices.push_back(static_cast<uint32_t>(i));
            edge_primitive.m_indices.push_back(static_cast<uint32_t>(i + 1));
        }
    } else {
        Standard_Real first = 0.0, last = 0.0;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, location, first, last);
        if(curve.IsNull()) {
            return;
        }
        GeomAdaptor_Curve gac(curve, first, last);
        GCPnts_UniformDeflection sampler(gac, std::max(options.m_linearDeflection, 1.0e-3), first,
                                         last);
        if(!sampler.IsDone() || sampler.NbPoints() < 2) {
            return;
        }
        const Standard_Integer count = sampler.NbPoints();
        edge_primitive.m_positions.reserve(static_cast<size_t>(count));
        edge_primitive.m_indices.reserve(
            static_cast<size_t>(std::max<Standard_Integer>(0, count - 1)) * 2);
        for(Standard_Integer i = 1; i <= count; ++i) {
            gp_Pnt point = sampler.Value(i);
            if(!isIdentityTrsf(trsf)) {
                point.Transform(trsf);
            }
            edge_primitive.m_positions.emplace_back(point.X(), point.Y(), point.Z());
            if(i > 1) {
                edge_primitive.m_indices.push_back(static_cast<uint32_t>(i - 2));
                edge_primitive.m_indices.push_back(static_cast<uint32_t>(i - 1));
            }
        }
    }

    if(edge_primitive.isValid()) {
        render_data.m_geometry.push_back(std::move(edge_primitive));
    }
    return;
}

void GeometryDocumentImpl::generateVertexMesh(Render::RenderData& render_data,
                                              const GeometryEntityImplPtr& entity,
                                              const Render::TessellationOptions& /*options*/) {
    if(!entity || !entity->hasShape()) {
        return;
    }

    const auto vertex = TopoDS::Vertex(entity->shape());

    Render::RenderPrimitive vertex_primitive;
    vertex_primitive.m_uid = entity->entityUID();
    vertex_primitive.m_entityType = Render::RenderEntityType::Vertex;
    vertex_primitive.m_topology = Render::PrimitiveTopology::Points;
    vertex_primitive.m_passType = Render::RenderPassType::Geometry;
    const gp_Pnt& pnt = BRep_Tool::Pnt(vertex);
    vertex_primitive.m_positions.emplace_back(pnt.X(), pnt.Y(), pnt.Z());
    if(vertex_primitive.isValid()) {
        render_data.m_geometry.push_back(std::move(vertex_primitive));
    }
    return;
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
