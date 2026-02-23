/**
 * @file geometry_documentImpl.cpp
 * @brief Implementation of GeometryDocument entity management
 */

#include "geometry_documentImpl.hpp"
#include "entity/geometry_entityImpl.hpp"
#include "geometry/part_color.hpp"
#include "shape_builder.hpp"
#include "util/logger.hpp"
#include "util/point_vector3d.hpp"
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

void computeSmoothVertexNormals(Render::RenderMesh& mesh) {
    if(mesh.m_vertices.empty() || mesh.m_indices.size() < 3) {
        return;
    }

    std::vector<Util::Vec3d> accum(mesh.m_vertices.size());
    for(size_t i = 0; i + 2 < mesh.m_indices.size(); i += 3) {
        const uint32_t i0 = mesh.m_indices[i + 0];
        const uint32_t i1 = mesh.m_indices[i + 1];
        const uint32_t i2 = mesh.m_indices[i + 2];
        if(i0 >= mesh.m_vertices.size() || i1 >= mesh.m_vertices.size() ||
           i2 >= mesh.m_vertices.size()) {
            continue;
        }

        const auto& v0 = mesh.m_vertices[i0];
        const auto& v1 = mesh.m_vertices[i1];
        const auto& v2 = mesh.m_vertices[i2];

        const Util::Vec3d p0{v0.m_position[0], v0.m_position[1], v0.m_position[2]};
        const Util::Vec3d p1{v1.m_position[0], v1.m_position[1], v1.m_position[2]};
        const Util::Vec3d p2{v2.m_position[0], v2.m_position[1], v2.m_position[2]};

        const Util::Vec3d n = (p1 - p0).cross(p2 - p0);
        accum[i0] += n;
        accum[i1] += n;
        accum[i2] += n;
    }

    for(size_t i = 0; i < mesh.m_vertices.size(); ++i) {
        const auto lsq = accum[i].squaredLength();
        if(lsq < 1e-24) {
            continue;
        }
        const double inv_len = 1.0 / std::sqrt(lsq);
        mesh.m_vertices[i].m_normal[0] = static_cast<float>(accum[i].x * inv_len);
        mesh.m_vertices[i].m_normal[1] = static_cast<float>(accum[i].y * inv_len);
        mesh.m_vertices[i].m_normal[2] = static_cast<float>(accum[i].z * inv_len);
    }
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

GeometryEntityPtr GeometryDocumentImpl::findByKey(const EntityKey& key) const {
    return m_entityIndex.findByKey(key);
}

GeometryEntityPtr GeometryDocumentImpl::findByRef(const EntityRef& ref) const {
    return m_entityIndex.findByRef(ref);
}

EntityRef GeometryDocumentImpl::resolveId(EntityId entity_id) const {
    return m_entityIndex.resolveId(entity_id);
}

EntityKey GeometryDocumentImpl::resolveIdToKey(EntityId entity_id) const {
    return m_entityIndex.resolveIdToKey(entity_id);
}

EntityKey GeometryDocumentImpl::resolveRefToKey(const EntityRef& ref) const {
    return m_entityIndex.resolveRefToKey(ref);
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

Render::DocumentRenderData
GeometryDocumentImpl::getRenderData(const Render::TessellationOptions& options) {
    std::lock_guard<std::mutex> lock(m_renderDataMutex);

    if(m_renderDataValid) {
        return m_cachedRenderData;
    }

    m_cachedRenderData.clear();
    m_cachedRenderData.m_faceBatch.m_primitiveType = Render::RenderPrimitiveType::Triangles;
    m_cachedRenderData.m_edgeBatch.m_primitiveType = Render::RenderPrimitiveType::Lines;
    m_cachedRenderData.m_vertexBatch.m_primitiveType = Render::RenderPrimitiveType::Points;

    auto faces = entitiesByType(EntityType::Face);
    LOG_DEBUG("getRenderData: Found {} faces in document", faces.size());
    for(const auto& face : faces) {
        appendFaceToBatch(m_cachedRenderData, face, options);
    }

    auto edges = entitiesByType(EntityType::Edge);
    LOG_DEBUG("getRenderData: Found {} edges in document", edges.size());
    for(const auto& edge : edges) {
        appendEdgeToBatch(m_cachedRenderData, edge, options);
    }

    auto vertices = entitiesByType(EntityType::Vertex);
    LOG_DEBUG("getRenderData: Found {} vertices in document", vertices.size());
    for(const auto& vertex : vertices) {
        appendVertexToBatch(m_cachedRenderData, vertex);
    }

    LOG_DEBUG("getRenderData: Batched faces={}, edges={}, vertices={} entities",
              m_cachedRenderData.m_faceEntities.size(), m_cachedRenderData.m_edgeEntities.size(),
              m_cachedRenderData.m_vertexEntities.size());
    m_cachedRenderData.updateBoundingBox();
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

std::vector<EntityKey> GeometryDocumentImpl::findRelatedEntities(const EntityRef& source,
                                                                 EntityType related_type) const {
    return m_relationshipIndex.findRelatedEntities(source.m_uid, source.m_type, related_type);
}

std::vector<EntityKey> GeometryDocumentImpl::findRelatedEntities(EntityUID entity_uid,
                                                                 EntityType entity_type,
                                                                 EntityType related_type) const {
    return m_relationshipIndex.findRelatedEntities(entity_uid, entity_type, related_type);
}
void GeometryDocumentImpl::appendFaceToBatch(Render::DocumentRenderData& data,
                                             const GeometryEntityImplPtr& entity,
                                             const Render::TessellationOptions& options) {
    const auto& shape = entity->shape();
    if(shape.IsNull()) {
        return;
    }

    // Ownership lookup
    auto parts = findRelatedEntities(entity->entityId(), EntityType::Part);
    if(parts.empty()) {
        return;
    }
    auto solids = findRelatedEntities(entity->entityId(), EntityType::Solid);
    auto wires = findRelatedEntities(entity->entityId(), EntityType::Wire);

    const auto owning_part_id = parts.front().m_id;
    PartColor face_color = PartColorPalette::getColorByEntityId(owning_part_id);

    const auto uid56 = entity->entityUID();
    const auto packed_uid = Render::RenderUID::encode(Render::RenderEntityType::Face, uid56);

    const double linear_deflection = std::max(1e-6, options.m_linearDeflection);
    BRepMesh_IncrementalMesh mesher(shape, linear_deflection, Standard_False,
                                    options.m_angularDeflection);

    TopLoc_Location loc;
    const Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(TopoDS::Face(shape), loc);
    if(tri.IsNull()) {
        return;
    }

    const gp_Trsf& trsf = loc.Transformation();
    const bool has_transform = !isIdentityTrsf(trsf);

    auto& batch = data.m_faceBatch;
    const auto base_vertex = static_cast<uint32_t>(batch.m_vertices.size());
    const auto base_index = static_cast<uint32_t>(batch.m_indices.size());

    // Vertices
    const Standard_Integer nb_nodes = tri->NbNodes();
    batch.m_vertices.reserve(batch.m_vertices.size() + static_cast<size_t>(nb_nodes));

    double cx = 0.0, cy = 0.0, cz = 0.0;

    for(Standard_Integer i = 1; i <= nb_nodes; ++i) {
        gp_Pnt pnt = tri->Node(i);
        if(has_transform) {
            pnt.Transform(trsf);
        }

        Render::RenderVertex vertex(static_cast<float>(pnt.X()), static_cast<float>(pnt.Y()),
                                    static_cast<float>(pnt.Z()));
        vertex.setColor(face_color.r, face_color.g, face_color.b, face_color.a);
        vertex.setUid(packed_uid.m_packed);

        if(options.m_computeNormals && tri->HasNormals()) {
            gp_Dir normal = tri->Normal(i);
            if(has_transform) {
                normal.Transform(trsf);
            }
            vertex.m_normal[0] = static_cast<float>(normal.X());
            vertex.m_normal[1] = static_cast<float>(normal.Y());
            vertex.m_normal[2] = static_cast<float>(normal.Z());
        }

        batch.m_vertices.push_back(vertex);
        batch.m_boundingBox.expand(Util::Pt3d(pnt.X(), pnt.Y(), pnt.Z()));

        cx += pnt.X();
        cy += pnt.Y();
        cz += pnt.Z();
    }

    // Indices (offset by base_vertex)
    const Standard_Integer nb_triangles = tri->NbTriangles();
    batch.m_indices.reserve(batch.m_indices.size() + static_cast<size_t>(nb_triangles) * 3);

    const TopAbs_Orientation orientation = shape.Orientation();

    for(Standard_Integer i = 1; i <= nb_triangles; ++i) {
        const Poly_Triangle& triangle = tri->Triangle(i);
        Standard_Integer n1, n2, n3;
        triangle.Get(n1, n2, n3);

        if(orientation == TopAbs_REVERSED) {
            std::swap(n2, n3);
        }

        batch.m_indices.push_back(base_vertex + static_cast<uint32_t>(n1 - 1));
        batch.m_indices.push_back(base_vertex + static_cast<uint32_t>(n2 - 1));
        batch.m_indices.push_back(base_vertex + static_cast<uint32_t>(n3 - 1));
    }

    // Compute smooth normals for this entity's range if OCC didn't provide them
    if(options.m_computeNormals && !tri->HasNormals()) {
        Render::RenderMesh temp;
        temp.m_vertices.assign(batch.m_vertices.begin() + base_vertex, batch.m_vertices.end());
        temp.m_indices.reserve(static_cast<size_t>(nb_triangles) * 3);
        for(size_t i = base_index; i < batch.m_indices.size(); ++i) {
            temp.m_indices.push_back(batch.m_indices[i] - base_vertex);
        }
        computeSmoothVertexNormals(temp);
        for(size_t i = 0; i < temp.m_vertices.size(); ++i) {
            batch.m_vertices[base_vertex + i].m_normal[0] = temp.m_vertices[i].m_normal[0];
            batch.m_vertices[base_vertex + i].m_normal[1] = temp.m_vertices[i].m_normal[1];
            batch.m_vertices[base_vertex + i].m_normal[2] = temp.m_vertices[i].m_normal[2];
        }
    }

    // Entity info
    const auto vertex_count = static_cast<uint32_t>(batch.m_vertices.size()) - base_vertex;
    const auto index_count = static_cast<uint32_t>(batch.m_indices.size()) - base_index;

    Render::RenderEntityInfo info;
    info.m_uid = packed_uid;
    info.m_indexOffset = base_index;
    info.m_indexCount = index_count;
    info.m_vertexOffset = base_vertex;
    info.m_vertexCount = vertex_count;

    info.m_owningPartUid56 = parts.front().m_uid;
    if(!solids.empty()) {
        info.m_owningSolidUid56 = solids.front().m_uid;
    }
    for(const auto& w : wires) {
        info.m_owningWireUid56s.push_back(w.m_uid);
    }

    info.m_hoverColor[0] = 0.310f;
    info.m_hoverColor[1] = 0.765f;
    info.m_hoverColor[2] = 0.969f;
    info.m_hoverColor[3] = face_color.a;

    info.m_selectedColor[0] = 0.118f;
    info.m_selectedColor[1] = 0.533f;
    info.m_selectedColor[2] = 0.898f;
    info.m_selectedColor[3] = face_color.a;

    if(nb_nodes > 0) {
        const double inv_n = 1.0 / static_cast<double>(nb_nodes);
        info.m_centroid[0] = static_cast<float>(cx * inv_n);
        info.m_centroid[1] = static_cast<float>(cy * inv_n);
        info.m_centroid[2] = static_cast<float>(cz * inv_n);
    }

    data.m_faceEntities[packed_uid.m_packed] = std::move(info);
}

void GeometryDocumentImpl::appendEdgeToBatch(Render::DocumentRenderData& data,
                                             const GeometryEntityImplPtr& entity,
                                             const Render::TessellationOptions& options) {
    const auto& shape = entity->shape();
    if(shape.IsNull()) {
        return;
    }

    const auto uid56 = entity->entityUID();
    const auto packed_uid = Render::RenderUID::encode(Render::RenderEntityType::Edge, uid56);

    const auto owning_parts = findRelatedEntities(entity->entityId(), EntityType::Part);
    const auto owning_solids = findRelatedEntities(entity->entityId(), EntityType::Solid);
    const auto owning_wires = findRelatedEntities(entity->entityId(), EntityType::Wire);

    constexpr float edge_color[4] = {1.0f, 0.8f, 0.2f, 1.0f};

    const TopoDS_Edge& edge = TopoDS::Edge(shape);
    const double linear_deflection = std::max(1e-6, options.m_linearDeflection);
    BRepMesh_IncrementalMesh mesher(shape, linear_deflection, Standard_False,
                                    options.m_angularDeflection);

    // Collect edge sample points
    std::vector<gp_Pnt> points;

    {
        TopLoc_Location loc;
        const Handle(Poly_Polygon3D) polygon = BRep_Tool::Polygon3D(edge, loc);
        if(!polygon.IsNull()) {
            const TColgp_Array1OfPnt& nodes = polygon->Nodes();
            const gp_Trsf& trsf = loc.Transformation();
            for(Standard_Integer i = nodes.Lower(); i <= nodes.Upper(); ++i) {
                gp_Pnt pnt = nodes(i);
                if(!isIdentityTrsf(trsf)) {
                    pnt.Transform(trsf);
                }
                points.push_back(pnt);
            }
        }
    }

    if(points.empty()) {
        Standard_Real first, last;
        TopLoc_Location curve_loc;
        const Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, curve_loc, first, last);
        if(curve.IsNull()) {
            return;
        }

        const gp_Trsf& curve_trsf = curve_loc.Transformation();
        const bool has_transform = !isIdentityTrsf(curve_trsf);

        GCPnts_UniformDeflection sampler(GeomAdaptor_Curve(curve, first, last), linear_deflection);
        if(!sampler.IsDone()) {
            return;
        }

        const Standard_Integer nb_points = sampler.NbPoints();
        points.reserve(static_cast<size_t>(nb_points));
        for(Standard_Integer i = 1; i <= nb_points; ++i) {
            gp_Pnt pnt = sampler.Value(i);
            if(has_transform) {
                pnt.Transform(curve_trsf);
            }
            points.push_back(pnt);
        }
    }

    if(points.size() < 2) {
        return;
    }

    auto& batch = data.m_edgeBatch;
    const auto base_vertex = static_cast<uint32_t>(batch.m_vertices.size());
    const auto base_index = static_cast<uint32_t>(batch.m_indices.size());

    // Append vertices
    double cx = 0.0, cy = 0.0, cz = 0.0;
    batch.m_vertices.reserve(batch.m_vertices.size() + points.size());
    for(const auto& pnt : points) {
        Render::RenderVertex vertex(static_cast<float>(pnt.X()), static_cast<float>(pnt.Y()),
                                    static_cast<float>(pnt.Z()));
        vertex.setColor(edge_color[0], edge_color[1], edge_color[2], edge_color[3]);
        vertex.setUid(packed_uid.m_packed);
        batch.m_vertices.push_back(vertex);
        batch.m_boundingBox.expand(Util::Pt3d(pnt.X(), pnt.Y(), pnt.Z()));

        cx += pnt.X();
        cy += pnt.Y();
        cz += pnt.Z();
    }

    // Generate indexed GL_LINES from consecutive points (LineStrip -> Lines)
    const auto n_pts = static_cast<uint32_t>(points.size());
    batch.m_indices.reserve(batch.m_indices.size() + static_cast<size_t>((n_pts - 1) * 2));
    for(uint32_t i = 0; i < n_pts - 1; ++i) {
        batch.m_indices.push_back(base_vertex + i);
        batch.m_indices.push_back(base_vertex + i + 1);
    }

    // Entity info
    const auto vertex_count = static_cast<uint32_t>(batch.m_vertices.size()) - base_vertex;
    const auto index_count = static_cast<uint32_t>(batch.m_indices.size()) - base_index;

    Render::RenderEntityInfo info;
    info.m_uid = packed_uid;
    info.m_indexOffset = base_index;
    info.m_indexCount = index_count;
    info.m_vertexOffset = base_vertex;
    info.m_vertexCount = vertex_count;

    if(!owning_parts.empty()) {
        info.m_owningPartUid56 = owning_parts.front().m_uid;
    }
    if(!owning_solids.empty()) {
        info.m_owningSolidUid56 = owning_solids.front().m_uid;
    }
    for(const auto& w : owning_wires) {
        info.m_owningWireUid56s.push_back(w.m_uid);
    }

    info.m_hoverColor[0] = 1.0f;
    info.m_hoverColor[1] = 0.6f;
    info.m_hoverColor[2] = 0.6f;
    info.m_hoverColor[3] = edge_color[3];

    info.m_selectedColor[0] = 1.0f;
    info.m_selectedColor[1] = 0.0f;
    info.m_selectedColor[2] = 0.0f;
    info.m_selectedColor[3] = edge_color[3];

    const double inv_n = 1.0 / static_cast<double>(points.size());
    info.m_centroid[0] = static_cast<float>(cx * inv_n);
    info.m_centroid[1] = static_cast<float>(cy * inv_n);
    info.m_centroid[2] = static_cast<float>(cz * inv_n);

    data.m_edgeEntities[packed_uid.m_packed] = std::move(info);
}

void GeometryDocumentImpl::appendVertexToBatch(Render::DocumentRenderData& data,
                                               const GeometryEntityImplPtr& entity) {
    const auto& shape = entity->shape();
    if(shape.IsNull()) {
        return;
    }

    const auto uid56 = entity->entityUID();
    const auto packed_uid = Render::RenderUID::encode(Render::RenderEntityType::Vertex, uid56);

    const auto owning_parts = findRelatedEntities(entity->entityId(), EntityType::Part);
    const auto owning_solids = findRelatedEntities(entity->entityId(), EntityType::Solid);
    const auto owning_wires = findRelatedEntities(entity->entityId(), EntityType::Wire);

    constexpr float vertex_color[4] = {0.2f, 1.0f, 0.4f, 1.0f};

    auto& batch = data.m_vertexBatch;
    const auto base_vertex = static_cast<uint32_t>(batch.m_vertices.size());

    const TopoDS_Vertex& vertex = TopoDS::Vertex(shape);
    const gp_Pnt pnt = BRep_Tool::Pnt(vertex);

    Render::RenderVertex render_vertex(static_cast<float>(pnt.X()), static_cast<float>(pnt.Y()),
                                       static_cast<float>(pnt.Z()));
    render_vertex.setColor(vertex_color[0], vertex_color[1], vertex_color[2], vertex_color[3]);
    render_vertex.setUid(packed_uid.m_packed);
    batch.m_vertices.push_back(render_vertex);
    batch.m_boundingBox.expand(Util::Pt3d(pnt.X(), pnt.Y(), pnt.Z()));

    Render::RenderEntityInfo info;
    info.m_uid = packed_uid;
    info.m_vertexOffset = base_vertex;
    info.m_vertexCount = 1;

    if(!owning_parts.empty()) {
        info.m_owningPartUid56 = owning_parts.front().m_uid;
    }
    if(!owning_solids.empty()) {
        info.m_owningSolidUid56 = owning_solids.front().m_uid;
    }
    for(const auto& w : owning_wires) {
        info.m_owningWireUid56s.push_back(w.m_uid);
    }

    info.m_hoverColor[0] = 1.0f;
    info.m_hoverColor[1] = 0.6f;
    info.m_hoverColor[2] = 0.0f;
    info.m_hoverColor[3] = vertex_color[3];

    info.m_selectedColor[0] = 1.0f;
    info.m_selectedColor[1] = 0.3f;
    info.m_selectedColor[2] = 0.0f;
    info.m_selectedColor[3] = vertex_color[3];

    info.m_centroid[0] = static_cast<float>(pnt.X());
    info.m_centroid[1] = static_cast<float>(pnt.Y());
    info.m_centroid[2] = static_cast<float>(pnt.Z());

    data.m_vertexEntities[packed_uid.m_packed] = std::move(info);
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
