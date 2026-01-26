/**
 * @file geometry_builder.cpp
 * @brief Implementation of GeometryBuilder for entity hierarchy construction
 */

#include "geometry/geometry_builder.hpp"

#include "geometry/comp_solid_entity.hpp"
#include "geometry/compound_entity.hpp"
#include "geometry/edge_entity.hpp"
#include "geometry/face_entity.hpp"
#include "geometry/shell_entity.hpp"
#include "geometry/solid_entity.hpp"
#include "geometry/vertex_entity.hpp"
#include "geometry/wire_entity.hpp"
#include "util/logger.hpp"

#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_CompSolid.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>

namespace OpenGeoLab::Geometry {

GeometryBuilder::GeometryBuilder(GeometryDocumentPtr document) : m_document(std::move(document)) {}

BuildResult GeometryBuilder::buildFromShape(const TopoDS_Shape& shape,
                                            const std::string& part_name,
                                            BuildProgressCallback progress_callback) {
    if(shape.IsNull()) {
        return BuildResult::failure("Input shape is null");
    }

    if(!m_document) {
        return BuildResult::failure("No document specified");
    }

    m_progressCallback = std::move(progress_callback);
    m_totalShapes = countSubShapes(shape);
    m_processedShapes = 0;

    if(!reportProgress("Starting entity hierarchy construction...")) {
        return BuildResult::failure("Operation cancelled");
    }

    LOG_DEBUG("Building entity hierarchy from shape with {} sub-shapes", m_totalShapes);

    // Create the part entity as the root container
    auto part_entity = std::make_shared<PartEntity>(shape);
    if(!part_name.empty()) {
        part_entity->setName(part_name);
    }

    if(!m_document->addEntity(part_entity)) {
        return BuildResult::failure("Failed to add part entity to document");
    }

    // Recursively build the entity hierarchy
    // Part's direct child is determined by the root shape type
    GeometryEntityPtr root_child = createEntityForShape(shape, part_entity);
    if(!root_child) {
        // Check if cancelled
        if(m_progressCallback && m_processedShapes < m_totalShapes) {
            return BuildResult::failure("Operation cancelled during hierarchy construction");
        }
        return BuildResult::failure("Failed to create entity hierarchy");
    }

    if(!reportProgress("Entity hierarchy construction complete")) {
        return BuildResult::failure("Operation cancelled");
    }

    LOG_INFO("Successfully built entity hierarchy: Part '{}' with {} entities",
             part_entity->name().empty() ? "Unnamed" : part_entity->name(),
             m_document->entityCount());

    return BuildResult::success(part_entity);
}

GeometryEntityPtr GeometryBuilder::createEntityForShape(const TopoDS_Shape& shape, // NOLINT
                                                        const GeometryEntityPtr& parent_entity) {
    if(shape.IsNull()) {
        return nullptr;
    }

    // Check for existing entity (shape deduplication)
    // Skip Part entities since they share shapes with their content
    auto existing = m_document->findByShape(shape);
    if(existing && existing->entityType() != EntityType::Part) {
        // Shape already has an entity; just link parent-child if not already linked
        if(parent_entity && !existing->hasParentId(parent_entity->entityId())) {
            (void)m_document->addChildEdge(parent_entity->entityId(), existing->entityId());
        }
        return existing;
    }

    // Create new entity for this shape
    auto entity = createTypedEntity(shape);
    if(!entity) {
        LOG_WARN("Could not create entity for shape type: {}", static_cast<int>(shape.ShapeType()));
        return nullptr;
    }

    // Register with document
    if(!m_document->addEntity(entity)) {
        LOG_ERROR("Failed to add entity to document");
        return nullptr;
    }

    // Link to parent
    if(parent_entity) {
        if(!m_document->addChildEdge(parent_entity->entityId(), entity->entityId())) {
            LOG_WARN("Failed to link entity {} to parent {}", entity->entityId(),
                     parent_entity->entityId());
        }
    }

    // Update progress
    ++m_processedShapes;
    if(!reportProgress("Processing shape " + std::to_string(m_processedShapes) + "/" +
                       std::to_string(m_totalShapes))) {
        return nullptr; // Cancelled
    }

    // Process children based on shape type
    TopAbs_ShapeEnum child_type;
    switch(shape.ShapeType()) {
    case TopAbs_COMPOUND:
        // Compound can contain any shape type, iterate directly
        for(TopoDS_Iterator it(shape); it.More(); it.Next()) {
            (void)createEntityForShape(it.Value(), entity);
        }
        break;

    case TopAbs_COMPSOLID:
        child_type = TopAbs_SOLID;
        for(TopExp_Explorer exp(shape, child_type); exp.More(); exp.Next()) {
            (void)createEntityForShape(exp.Current(), entity);
        }
        break;

    case TopAbs_SOLID:
        child_type = TopAbs_SHELL;
        for(TopExp_Explorer exp(shape, child_type); exp.More(); exp.Next()) {
            (void)createEntityForShape(exp.Current(), entity);
        }
        break;

    case TopAbs_SHELL:
        child_type = TopAbs_FACE;
        for(TopExp_Explorer exp(shape, child_type); exp.More(); exp.Next()) {
            (void)createEntityForShape(exp.Current(), entity);
        }
        break;

    case TopAbs_FACE:
        child_type = TopAbs_WIRE;
        for(TopExp_Explorer exp(shape, child_type); exp.More(); exp.Next()) {
            (void)createEntityForShape(exp.Current(), entity);
        }
        break;

    case TopAbs_WIRE:
        child_type = TopAbs_EDGE;
        for(TopExp_Explorer exp(shape, child_type); exp.More(); exp.Next()) {
            (void)createEntityForShape(exp.Current(), entity);
        }
        break;

    case TopAbs_EDGE:
        child_type = TopAbs_VERTEX;
        for(TopExp_Explorer exp(shape, child_type); exp.More(); exp.Next()) {
            (void)createEntityForShape(exp.Current(), entity);
        }
        break;

    case TopAbs_VERTEX:
        // Leaf node, no children
        break;

    default:
        break;
    }

    return entity;
}

GeometryEntityPtr GeometryBuilder::createTypedEntity(const TopoDS_Shape& shape) {
    switch(shape.ShapeType()) {
    case TopAbs_COMPOUND:
        return std::make_shared<CompoundEntity>(TopoDS::Compound(shape));

    case TopAbs_COMPSOLID:
        return std::make_shared<CompSolidEntity>(TopoDS::CompSolid(shape));

    case TopAbs_SOLID:
        return std::make_shared<SolidEntity>(TopoDS::Solid(shape));

    case TopAbs_SHELL:
        return std::make_shared<ShellEntity>(TopoDS::Shell(shape));

    case TopAbs_FACE:
        return std::make_shared<FaceEntity>(TopoDS::Face(shape));

    case TopAbs_WIRE:
        return std::make_shared<WireEntity>(TopoDS::Wire(shape));

    case TopAbs_EDGE:
        return std::make_shared<EdgeEntity>(TopoDS::Edge(shape));

    case TopAbs_VERTEX:
        return std::make_shared<VertexEntity>(TopoDS::Vertex(shape));

    default:
        return nullptr;
    }
}

size_t GeometryBuilder::countSubShapes(const TopoDS_Shape& shape) const { // NOLINT
    if(shape.IsNull()) {
        return 0;
    }

    size_t count = 1; // Count the shape itself

    // Count immediate children only (not recursive to avoid double counting)
    // The actual traversal will handle deduplication
    for(TopoDS_Iterator it(shape); it.More(); it.Next()) {
        ++count;
        count += countSubShapes(it.Value());
    }

    return count;
}

bool GeometryBuilder::reportProgress(const std::string& message) {
    if(!m_progressCallback) {
        return true;
    }

    double progress = m_totalShapes > 0 ? static_cast<double>(m_processedShapes) /
                                              static_cast<double>(m_totalShapes)
                                        : 0.0;

    return m_progressCallback(progress, message);
}

} // namespace OpenGeoLab::Geometry
