/**
 * @file geometry_document.cpp
 * @brief Implementation of GeometryDocument entity management
 */

#include "geometry_documentImpl.hpp"
#include "entity/geometry_entity.hpp"
#include "shape_builder.hpp"
#include "util/logger.hpp"
#include "util/progress_callback.hpp"

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>

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

void GeometryDocumentImpl::clear() { m_entityIndex.clear(); }

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
    if(shape.IsNull()) {
        return LoadResult::failure("Input shape is null");
    }

    if(!progress(0.0, "Starting shape load...")) {
        return LoadResult::failure("Operation cancelled");
    }

    try {
        ShapeBuilder builder(shared_from_this());

        auto subcallback = Util::makeScaledProgressCallback(progress, 0.0, 0.9);

        auto build_result = builder.buildFromShape(shape, name, subcallback);

        if(!build_result.m_success) {
            return LoadResult::failure(build_result.m_errorMessage);
        }

        if(!progress(0.95, "Finalizing...")) {
            return LoadResult::failure("Operation cancelled");
        }

        progress(1.0, "Load completed.");
        return LoadResult::success(build_result.m_rootPart->entityId(),
                                   build_result.totalEntityCount());
    } catch(const std::exception& e) {
        LOG_ERROR("Exception during shape load: {}", e.what());
        return LoadResult::failure(std::string("Exception: ") + e.what());
    }
}

Render::RenderScene GeometryDocumentImpl::generateRenderScene(double deflection) const {
    Render::RenderScene scene;

    // Get all face entities for tessellation
    auto faces = entitiesByType(EntityType::Face);
    for(const auto& face : faces) {
        auto mesh = generateRenderMesh(face->entityId(), deflection);
        if(mesh.isValid()) {
            scene.m_meshes.push_back(std::move(mesh));
        }
    }

    scene.updateBoundingBox();
    LOG_DEBUG("Generated render scene with {} meshes", scene.m_meshes.size());
    return scene;
}

Render::RenderMesh GeometryDocumentImpl::generateRenderMesh(EntityId entity_id,
                                                            double deflection) const {
    Render::RenderMesh mesh;
    mesh.m_entityId = entity_id;

    auto entity = findById(entity_id);
    if(!entity || !entity->hasShape()) {
        return mesh;
    }

    const TopoDS_Shape& shape = entity->shape();

    // Use OCC's tessellation
    BRepMesh_IncrementalMesh mesher(shape, deflection);
    if(!mesher.IsDone()) {
        LOG_WARN("Tessellation failed for entity {}", entity_id);
        return mesh;
    }

    // Extract triangulation from faces
    if(entity->entityType() == EntityType::Face) {
        TopLoc_Location location;
        Handle(Poly_Triangulation) triangulation =
            BRep_Tool::Triangulation(TopoDS::Face(shape), location);

        if(triangulation.IsNull()) {
            return mesh;
        }

        gp_Trsf transform = location.Transformation();
        bool needsTransform = !location.IsIdentity();

        // Reserve space
        int nbNodes = triangulation->NbNodes();
        int nbTriangles = triangulation->NbTriangles();
        mesh.m_vertices.reserve(static_cast<size_t>(nbNodes));
        mesh.m_indices.reserve(static_cast<size_t>(nbTriangles * 3));

        // Check face orientation
        TopAbs_Orientation orientation = shape.Orientation();
        bool reversed = (orientation == TopAbs_REVERSED);

        // Extract vertices with normals
        for(int i = 1; i <= nbNodes; ++i) {
            gp_Pnt point = triangulation->Node(i);
            if(needsTransform) {
                point.Transform(transform);
            }

            Render::RenderVertex vertex(static_cast<float>(point.X()),
                                        static_cast<float>(point.Y()),
                                        static_cast<float>(point.Z()));

            // Get normal if available
            if(triangulation->HasNormals()) {
                gp_Dir normal = triangulation->Normal(i);
                if(needsTransform) {
                    normal.Transform(transform);
                }
                if(reversed) {
                    normal.Reverse();
                }
                vertex.m_normal[0] = static_cast<float>(normal.X());
                vertex.m_normal[1] = static_cast<float>(normal.Y());
                vertex.m_normal[2] = static_cast<float>(normal.Z());
            }

            mesh.m_vertices.push_back(vertex);
        }

        // Extract triangles
        for(int i = 1; i <= nbTriangles; ++i) {
            int n1, n2, n3;
            triangulation->Triangle(i).Get(n1, n2, n3);

            // Adjust for reversed face
            if(reversed) {
                std::swap(n2, n3);
            }

            mesh.m_indices.push_back(static_cast<uint32_t>(n1 - 1));
            mesh.m_indices.push_back(static_cast<uint32_t>(n2 - 1));
            mesh.m_indices.push_back(static_cast<uint32_t>(n3 - 1));
        }

        mesh.m_primitiveType = Render::RenderPrimitiveType::Triangles;
    }

    return mesh;
}

} // namespace OpenGeoLab::Geometry
