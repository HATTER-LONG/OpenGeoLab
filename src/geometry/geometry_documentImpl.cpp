/**
 * @file geometry_documentImpl.cpp
 * @brief Implementation of GeometryDocument entity management
 */

#include "geometry_documentImpl.hpp"
#include "entity/geometry_entity.hpp"
#include "geometry/part_color.hpp"
#include "shape_builder.hpp"
#include "util/logger.hpp"
#include "util/progress_callback.hpp"

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GCPnts_UniformAbscissa.hxx>
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

#include <queue>
#include <unordered_set>

namespace OpenGeoLab::Geometry {

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

bool GeometryDocumentImpl::addEntity(const GeometryEntityPtr& entity) {
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
    const size_t count = m_entityIndex.entityCount();
    m_entityIndex.clear();
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

GeometryEntityPtr GeometryDocumentImpl::findByShape(const TopoDS_Shape& shape) const {
    return m_entityIndex.findByShape(shape);
}

[[nodiscard]] size_t GeometryDocumentImpl::entityCount() const {
    return m_entityIndex.entityCount();
}

[[nodiscard]] size_t GeometryDocumentImpl::entityCountByType(EntityType entity_type) const {
    return m_entityIndex.entityCountByType(entity_type);
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

    // Generate face meshes
    auto faces = entitiesByType(EntityType::Face);
    LOG_DEBUG("getRenderData: Found {} faces in document", faces.size());
    for(const auto& face : faces) {
        auto mesh = generateFaceMesh(face, options);
        if(mesh.isValid()) {
            m_cachedRenderData.m_faceMeshes.push_back(std::move(mesh));
        }
    }

    // Generate edge meshes
    auto edges = entitiesByType(EntityType::Edge);
    LOG_DEBUG("getRenderData: Found {} edges in document", edges.size());
    for(const auto& edge : edges) {
        auto mesh = generateEdgeMesh(edge, options);
        if(mesh.isValid()) {
            m_cachedRenderData.m_edgeMeshes.push_back(std::move(mesh));
        }
    }

    // Generate vertex meshes
    auto vertices = entitiesByType(EntityType::Vertex);
    LOG_DEBUG("getRenderData: Found {} vertices in document", vertices.size());
    for(const auto& vertex : vertices) {
        auto mesh = generateVertexMesh(vertex);
        if(mesh.isValid()) {
            m_cachedRenderData.m_vertexMeshes.push_back(std::move(mesh));
        }
    }
    LOG_DEBUG("getRenderData: Generated {} face meshes, {} edge meshes, {} vertex meshes",
              m_cachedRenderData.m_faceMeshes.size(), m_cachedRenderData.m_edgeMeshes.size(),
              m_cachedRenderData.m_vertexMeshes.size());
    m_cachedRenderData.updateBoundingBox();
    m_renderDataValid = true;

    return m_cachedRenderData;
}

void GeometryDocumentImpl::invalidateRenderData() {
    std::lock_guard<std::mutex> lock(m_renderDataMutex);
    m_renderDataValid = false;
}

Render::RenderMesh
GeometryDocumentImpl::generateFaceMesh(const GeometryEntityPtr& entity,
                                       const Render::TessellationOptions& options) {
    Render::RenderMesh mesh;
    mesh.m_entityId = entity->entityId();
    mesh.m_entityUid = entity->entityUID();
    mesh.m_entityType = EntityType::Face;
    mesh.m_primitiveType = Render::RenderPrimitiveType::Triangles;

    const auto& shape = entity->shape();
    if(shape.IsNull()) {
        return mesh;
    }

    // Determine face color based on owning part
    PartColor face_color(0.7f, 0.7f, 0.7f, 1.0f); // Default gray
    auto owning_part = findOwningPart(entity->entityId());
    if(owning_part) {
        // Use entity ID for consistent color assignment
        face_color = PartColorPalette::getColorByEntityId(owning_part->entityId());
    }

    // Use OCC's triangulation (already computed by BRepMesh)
    TopLoc_Location loc;
    const Handle(Poly_Triangulation) triangulation =
        BRep_Tool::Triangulation(TopoDS::Face(shape), loc);

    if(triangulation.IsNull()) {
        // Try to mesh the face first
        BRepMesh_IncrementalMesh mesher(shape, options.m_linearDeflection, Standard_False,
                                        options.m_angularDeflection);

        const Handle(Poly_Triangulation) tri2 = BRep_Tool::Triangulation(TopoDS::Face(shape), loc);
        if(tri2.IsNull()) {
            return mesh;
        }
    }

    const Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(TopoDS::Face(shape), loc);
    if(tri.IsNull()) {
        return mesh;
    }

    const gp_Trsf& trsf = loc.Transformation();
    const bool has_transform = !isIdentityTrsf(trsf);

    // Extract vertices
    const Standard_Integer nb_nodes = tri->NbNodes();
    mesh.m_vertices.reserve(static_cast<size_t>(nb_nodes));

    for(Standard_Integer i = 1; i <= nb_nodes; ++i) {
        gp_Pnt pnt = tri->Node(i);
        if(has_transform) {
            pnt.Transform(trsf);
        }

        Render::RenderVertex vertex(static_cast<float>(pnt.X()), static_cast<float>(pnt.Y()),
                                    static_cast<float>(pnt.Z()));

        // Set face color based on owning part
        vertex.setColor(face_color.r, face_color.g, face_color.b, face_color.a);

        // Get normal if available
        if(options.m_computeNormals && tri->HasNormals()) {
            gp_Dir normal = tri->Normal(i);
            if(has_transform) {
                normal.Transform(trsf);
            }
            vertex.m_normal[0] = static_cast<float>(normal.X());
            vertex.m_normal[1] = static_cast<float>(normal.Y());
            vertex.m_normal[2] = static_cast<float>(normal.Z());
        }

        mesh.m_vertices.push_back(vertex);
        mesh.m_boundingBox.expand(Point3D(pnt.X(), pnt.Y(), pnt.Z()));
    }

    // Extract triangles
    const Standard_Integer nb_triangles = tri->NbTriangles();
    mesh.m_indices.reserve(static_cast<size_t>(nb_triangles) * 3);

    const TopAbs_Orientation orientation = shape.Orientation();

    for(Standard_Integer i = 1; i <= nb_triangles; ++i) {
        const Poly_Triangle& triangle = tri->Triangle(i);
        Standard_Integer n1, n2, n3;
        triangle.Get(n1, n2, n3);

        // Adjust winding based on face orientation
        if(orientation == TopAbs_REVERSED) {
            std::swap(n2, n3);
        }

        mesh.m_indices.push_back(static_cast<uint32_t>(n1 - 1));
        mesh.m_indices.push_back(static_cast<uint32_t>(n2 - 1));
        mesh.m_indices.push_back(static_cast<uint32_t>(n3 - 1));
    }

    return mesh;
}

Render::RenderMesh
GeometryDocumentImpl::generateEdgeMesh(const GeometryEntityPtr& entity,
                                       const Render::TessellationOptions& options) {
    Render::RenderMesh mesh;
    mesh.m_entityId = entity->entityId();
    mesh.m_entityUid = entity->entityUID();
    mesh.m_entityType = EntityType::Edge;
    mesh.m_primitiveType = Render::RenderPrimitiveType::LineStrip;

    // Edge color: yellow for visibility
    constexpr float edge_color[4] = {1.0f, 0.8f, 0.2f, 1.0f};

    const auto& shape = entity->shape();
    if(shape.IsNull()) {
        return mesh;
    }

    const TopoDS_Edge& edge = TopoDS::Edge(shape);
    Standard_Real first, last;
    const Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);

    if(curve.IsNull()) {
        // Try polygon on triangulation
        TopLoc_Location loc;
        const Handle(Poly_Polygon3D) polygon = BRep_Tool::Polygon3D(edge, loc);
        if(!polygon.IsNull()) {
            const TColgp_Array1OfPnt& nodes = polygon->Nodes();
            const gp_Trsf& trsf = loc.Transformation();

            for(Standard_Integer i = nodes.Lower(); i <= nodes.Upper(); ++i) {
                gp_Pnt pnt = nodes(i);
                pnt.Transform(trsf);
                auto& vertex = mesh.m_vertices.emplace_back(static_cast<float>(pnt.X()),
                                                            static_cast<float>(pnt.Y()),
                                                            static_cast<float>(pnt.Z()));
                vertex.setColor(edge_color[0], edge_color[1], edge_color[2], edge_color[3]);
                mesh.m_boundingBox.expand(Point3D(pnt.X(), pnt.Y(), pnt.Z()));
            }
        }
        return mesh;
    }

    // Sample the curve with improved quality for curved edges
    // Use adaptive deflection based on curve length for better circular arc display
    GeomAdaptor_Curve adaptor(curve, first, last);

    // Calculate a good deflection based on curve characteristics
    double curve_length = GCPnts_AbscissaPoint::Length(adaptor);
    double effective_deflection =
        std::min(options.m_linearDeflection, curve_length / options.m_minEdgePoints);

    GCPnts_UniformDeflection sampler(adaptor, effective_deflection);

    if(!sampler.IsDone()) {
        // Fallback: use uniform abscissa sampling for problematic curves
        GCPnts_UniformAbscissa uniform_sampler(adaptor, options.m_minEdgePoints);
        if(uniform_sampler.IsDone()) {
            const Standard_Integer nb_pts = uniform_sampler.NbPoints();
            mesh.m_vertices.reserve(static_cast<size_t>(nb_pts));
            for(Standard_Integer i = 1; i <= nb_pts; ++i) {
                gp_Pnt pnt;
                adaptor.D0(uniform_sampler.Parameter(i), pnt);
                auto& vertex = mesh.m_vertices.emplace_back(static_cast<float>(pnt.X()),
                                                            static_cast<float>(pnt.Y()),
                                                            static_cast<float>(pnt.Z()));
                vertex.setColor(edge_color[0], edge_color[1], edge_color[2], edge_color[3]);
                mesh.m_boundingBox.expand(Point3D(pnt.X(), pnt.Y(), pnt.Z()));
            }
        }
        return mesh;
    }

    const Standard_Integer nb_points = sampler.NbPoints();
    mesh.m_vertices.reserve(static_cast<size_t>(nb_points));

    for(Standard_Integer i = 1; i <= nb_points; ++i) {
        const gp_Pnt& pnt = sampler.Value(i);
        auto& vertex = mesh.m_vertices.emplace_back(
            static_cast<float>(pnt.X()), static_cast<float>(pnt.Y()), static_cast<float>(pnt.Z()));
        vertex.setColor(edge_color[0], edge_color[1], edge_color[2], edge_color[3]);
        mesh.m_boundingBox.expand(Point3D(pnt.X(), pnt.Y(), pnt.Z()));
    }

    return mesh;
}

Render::RenderMesh GeometryDocumentImpl::generateVertexMesh(const GeometryEntityPtr& entity) {
    Render::RenderMesh mesh;
    mesh.m_entityId = entity->entityId();
    mesh.m_entityUid = entity->entityUID();
    mesh.m_entityType = EntityType::Vertex;
    mesh.m_primitiveType = Render::RenderPrimitiveType::Points;
    constexpr float vertex_color[4] = {0.2f, 1.0f, 0.4f, 1.0f};
    const auto& shape = entity->shape();
    if(shape.IsNull()) {
        return mesh;
    }

    const TopoDS_Vertex& vertex = TopoDS::Vertex(shape);
    const gp_Pnt pnt = BRep_Tool::Pnt(vertex);

    auto& render_vertex = mesh.m_vertices.emplace_back(
        static_cast<float>(pnt.X()), static_cast<float>(pnt.Y()), static_cast<float>(pnt.Z()));
    render_vertex.setColor(vertex_color[0], vertex_color[1], vertex_color[2], vertex_color[3]);

    mesh.m_boundingBox.expand(Point3D(pnt.X(), pnt.Y(), pnt.Z()));

    return mesh;
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
