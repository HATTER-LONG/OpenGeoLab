/**
 * @file geometry_documentImpl.cpp
 * @brief Implementation of GeometryDocument entity management
 */

#include "geometry_documentImpl.hpp"
#include "entity/geometry_entityImpl.hpp"
#include "geometry/part_color.hpp"
#include "shape_builder.hpp"
#include "util/logger.hpp"
#include "util/progress_callback.hpp"

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_UniformDeflection.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Geom_Curve.hxx>
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

#include <algorithm>

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

Render::RenderColor toRenderColor(const PartColor& color) {
    return Render::RenderColor{color.r, color.g, color.b, color.a};
}

void appendPointToPrimitive(Render::RenderPrimitive& primitive, const gp_Pnt& point) {
    primitive.m_positions.push_back(static_cast<float>(point.X()));
    primitive.m_positions.push_back(static_cast<float>(point.Y()));
    primitive.m_positions.push_back(static_cast<float>(point.Z()));
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

        auto subcallback = Util::makeScaledProgressCallback(progress, 0.1, 0.9);

        auto build_result = builder.buildFromShape(shape, name, subcallback);

        if(!build_result.m_success) {
            LOG_ERROR("GeometryDocument: Shape build failed: {}", build_result.m_errorMessage);
            return LoadResult::failure(build_result.m_errorMessage);
        }

        if(!progress(0.95, "Finalizing...")) {
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

// =============================================================================
// Render Data Implementation
// =============================================================================

const Render::RenderData&
GeometryDocumentImpl::getRenderData(const Render::TessellationOptions& options) {
    std::lock_guard<std::mutex> lock(m_renderDataMutex);

    if(m_renderDataValid) {
        return m_cachedRenderData;
    }

    m_cachedRenderData.clear();

    for(const auto& entity : entitiesByType(EntityType::Face)) {
        auto face_data = generateFaceMesh(entity, options);
        m_cachedRenderData.m_primitives.insert(m_cachedRenderData.m_primitives.end(),
                                               face_data.m_primitives.begin(),
                                               face_data.m_primitives.end());
    }

    for(const auto& entity : entitiesByType(EntityType::Edge)) {
        auto edge_data = generateEdgeMesh(entity, options);
        m_cachedRenderData.m_primitives.insert(m_cachedRenderData.m_primitives.end(),
                                               edge_data.m_primitives.begin(),
                                               edge_data.m_primitives.end());
    }

    for(const auto& entity : entitiesByType(EntityType::Vertex)) {
        auto vertex_data = generateVertexMesh(entity);
        m_cachedRenderData.m_primitives.insert(m_cachedRenderData.m_primitives.end(),
                                               vertex_data.m_primitives.begin(),
                                               vertex_data.m_primitives.end());
    }

    m_renderDataValid = true;

    return m_cachedRenderData;
}

void GeometryDocumentImpl::invalidateRenderData() {
    std::lock_guard<std::mutex> lock(m_renderDataMutex);
    m_renderDataValid = false;
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

Render::RenderData
GeometryDocumentImpl::generateFaceMesh(const GeometryEntityImplPtr& entity,
                                       const Render::TessellationOptions& options) {
    Render::RenderData data;
    if(!entity || !entity->hasShape()) {
        return data;
    }

    const auto face = TopoDS::Face(entity->shape());
    if(face.IsNull()) {
        return data;
    }

    BRepMesh_IncrementalMesh mesher(face, options.m_linearDeflection, false,
                                    options.m_angularDeflection, false);
    (void)mesher;

    TopLoc_Location location;
    const Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);
    if(triangulation.IsNull() || triangulation->NbNodes() < 3 || triangulation->NbTriangles() < 1) {
        return data;
    }

    Render::RenderPrimitive primitive;
    primitive.m_pass = Render::RenderPassType::Geometry;
    primitive.m_entityType = Render::RenderEntityType::Face;
    primitive.m_topology = Render::PrimitiveTopology::Triangles;
    primitive.m_color = toRenderColor(
        PartColorPalette::getColorByEntityId(static_cast<EntityId>(entity->entityId())));

    const gp_Trsf trsf = location.Transformation();
    for(Standard_Integer node_index = 1; node_index <= triangulation->NbNodes(); ++node_index) {
        gp_Pnt point = triangulation->Node(node_index);
        if(!isIdentityTrsf(trsf)) {
            point.Transform(trsf);
        }
        appendPointToPrimitive(primitive, point);
    }

    for(Standard_Integer tri_index = 1; tri_index <= triangulation->NbTriangles(); ++tri_index) {
        Poly_Triangle triangle = triangulation->Triangle(tri_index);
        Standard_Integer n1 = 0;
        Standard_Integer n2 = 0;
        Standard_Integer n3 = 0;
        triangle.Get(n1, n2, n3);

        if(face.Orientation() == TopAbs_REVERSED) {
            std::swap(n2, n3);
        }

        primitive.m_indices.push_back(static_cast<uint32_t>(n1 - 1));
        primitive.m_indices.push_back(static_cast<uint32_t>(n2 - 1));
        primitive.m_indices.push_back(static_cast<uint32_t>(n3 - 1));
    }

    if(!primitive.empty()) {
        data.m_primitives.push_back(std::move(primitive));
    }
    return data;
}

Render::RenderData
GeometryDocumentImpl::generateEdgeMesh(const GeometryEntityImplPtr& entity,
                                       const Render::TessellationOptions& options) {
    Render::RenderData data;
    if(!entity || !entity->hasShape()) {
        return data;
    }

    const auto edge = TopoDS::Edge(entity->shape());
    if(edge.IsNull()) {
        return data;
    }

    Render::RenderPrimitive primitive;
    primitive.m_pass = Render::RenderPassType::Geometry;
    primitive.m_entityType = Render::RenderEntityType::Edge;
    primitive.m_topology = Render::PrimitiveTopology::Lines;
    primitive.m_color = Render::RenderColor{0.12f, 0.12f, 0.12f, 1.0f};

    TopLoc_Location location;
    const Handle(Poly_Polygon3D) polygon = BRep_Tool::Polygon3D(edge, location);
    const gp_Trsf trsf = location.Transformation();

    if(!polygon.IsNull() && polygon->NbNodes() >= 2) {
        const auto& nodes = polygon->Nodes();
        const Standard_Integer lower = nodes.Lower();
        const Standard_Integer upper = nodes.Upper();

        for(Standard_Integer index = lower; index <= upper; ++index) {
            gp_Pnt point = nodes.Value(index);
            if(!isIdentityTrsf(trsf)) {
                point.Transform(trsf);
            }
            appendPointToPrimitive(primitive, point);
        }

        for(Standard_Integer index = 1; index < (upper - lower + 1); ++index) {
            primitive.m_indices.push_back(static_cast<uint32_t>(index - 1));
            primitive.m_indices.push_back(static_cast<uint32_t>(index));
        }
    } else {
        Standard_Real first_param = 0.0;
        Standard_Real last_param = 0.0;
        const Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first_param, last_param);
        if(curve.IsNull()) {
            return data;
        }

        GeomAdaptor_Curve adaptor(curve, first_param, last_param);
        GCPnts_UniformDeflection sampler(adaptor, std::max(options.m_linearDeflection, 1.0e-3),
                                         first_param, last_param);

        if(!sampler.IsDone() || sampler.NbPoints() < 2) {
            return data;
        }

        for(Standard_Integer index = 1; index <= sampler.NbPoints(); ++index) {
            gp_Pnt point = sampler.Value(index);
            if(!isIdentityTrsf(trsf)) {
                point.Transform(trsf);
            }
            appendPointToPrimitive(primitive, point);
            if(index > 1) {
                primitive.m_indices.push_back(static_cast<uint32_t>(index - 2));
                primitive.m_indices.push_back(static_cast<uint32_t>(index - 1));
            }
        }
    }

    if(primitive.m_positions.size() >= 6) {
        data.m_primitives.push_back(std::move(primitive));
    }

    return data;
}

Render::RenderData GeometryDocumentImpl::generateVertexMesh(const GeometryEntityImplPtr& entity) {
    Render::RenderData data;
    if(!entity || !entity->hasShape()) {
        return data;
    }

    const auto vertex = TopoDS::Vertex(entity->shape());
    if(vertex.IsNull()) {
        return data;
    }

    Render::RenderPrimitive primitive;
    primitive.m_pass = Render::RenderPassType::Geometry;
    primitive.m_entityType = Render::RenderEntityType::Vertex;
    primitive.m_topology = Render::PrimitiveTopology::Points;
    primitive.m_color = Render::RenderColor{0.92f, 0.52f, 0.08f, 1.0f};

    appendPointToPrimitive(primitive, BRep_Tool::Pnt(vertex));

    if(!primitive.empty()) {
        data.m_primitives.push_back(std::move(primitive));
    }
    return data;
}

// =============================================================================
// Change Notification Implementation
// =============================================================================

Util::ScopedConnection
GeometryDocumentImpl::subscribeToChanges(std::function<void(const GeometryChangeEvent&)> callback) {
    return m_changeSignal.connect(std::move(callback));
}

void GeometryDocumentImpl::emitChangeEvent(const GeometryChangeEvent& event) {
    invalidateRenderData();
    m_changeSignal.emitSignal(event);
}

} // namespace OpenGeoLab::Geometry
