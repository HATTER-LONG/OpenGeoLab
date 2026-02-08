/**
 * @file shape_builder.cpp
 * @brief Implementation of ShapeBuilder for creating entity hierarchies
 */

#include "shape_builder.hpp"
#include "entity/comp_solid_entity.hpp"
#include "entity/compound_entity.hpp"
#include "entity/edge_entity.hpp"
#include "entity/face_entity.hpp"
#include "entity/shell_entity.hpp"
#include "entity/solid_entity.hpp"
#include "entity/vertex_entity.hpp"
#include "entity/wire_entity.hpp"
#include "util/logger.hpp"

#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Iterator.hxx>

namespace OpenGeoLab::Geometry {

ShapeBuilder::ShapeBuilder(GeometryDocumentImplPtr document) : m_document(std::move(document)) {
    if(!m_document) {
        throw std::invalid_argument("ShapeBuilder requires a valid GeometryDocument");
    }
}

ShapeBuildResult ShapeBuilder::buildFromShape(const TopoDS_Shape& shape,
                                              const std::string& part_name,
                                              Util::ProgressCallback progress_callback) {
    if(shape.IsNull()) {
        return ShapeBuildResult::failure("Input shape is null");
    }

    if(!progress_callback(0.0, "Starting shape build...")) {
        return ShapeBuildResult::failure("Operation cancelled");
    }

    // Create the root Part entity
    auto part = std::make_shared<PartEntity>(shape);
    part->setName(part_name.empty() ? "Part" : part_name);

    if(!m_document->addEntity(part)) {
        return ShapeBuildResult::failure("Failed to add part entity to document");
    }

    ShapeBuildResult result = ShapeBuildResult::success(part);

    if(!progress_callback(0.1, "Indexing shapes...")) {
        (void)m_document->removeEntity(part->entityId());
        return ShapeBuildResult::failure("Operation cancelled");
    }

    // Build indexed map of all unique shapes
    // This ensures each topological element is only processed once
    TopTools_IndexedMapOfShape shape_map;
    TopExp::MapShapes(shape, shape_map);

    LOG_DEBUG("ShapeBuilder: Found {} unique shapes in model", shape_map.Extent());

    if(!progress_callback(0.2, "Creating entities...")) {
        (void)m_document->removeEntity(part->entityId());
        return ShapeBuildResult::failure("Operation cancelled");
    }

    // Create entities for all unique shapes
    ShapeEntityMap shape_entity_map;
    buildEntitiesFromShapeMap(shape_map, shape_entity_map, result);

    if(!progress_callback(0.6, "Building relationships...")) {
        (void)m_document->removeEntity(part->entityId());
        return ShapeBuildResult::failure("Operation cancelled");
    }

    // Build parent-child relationships
    buildRelationships(shape, shape_map, shape_entity_map, part);

    // Build derived caches for fast relationship queries.
    (void)m_document->relationships().buildRelationships();

    if(!progress_callback(1.0, "Shape build completed.")) {
        // Don't fail on final progress report
    }

    LOG_INFO("ShapeBuilder: Created {} entities (V:{} E:{} W:{} F:{} Sh:{} So:{} C:{})",
             result.totalEntityCount(), result.m_vertexCount, result.m_edgeCount,
             result.m_wireCount, result.m_faceCount, result.m_shellCount, result.m_solidCount,
             result.m_compoundCount);

    return result;
}

void ShapeBuilder::buildEntitiesFromShapeMap(const TopTools_IndexedMapOfShape& shape_map,
                                             ShapeEntityMap& shape_entity_map,
                                             ShapeBuildResult& result) {
    const int count = shape_map.Extent();

    for(int i = 1; i <= count; ++i) {
        const TopoDS_Shape& sub_shape = shape_map(i);

        auto entity = createEntityForShape(sub_shape);
        if(!entity) {
            LOG_WARN("ShapeBuilder: Skipping unsupported shape type at index {}", i);
            continue;
        }

        if(!m_document->addEntity(entity)) {
            LOG_ERROR("ShapeBuilder: Failed to add entity at index {} to document", i);
            continue;
        }

        shape_entity_map[i] = entity;
        updateEntityCounts(sub_shape, result);
    }
}

void ShapeBuilder::buildRelationships(const TopoDS_Shape& root_shape,
                                      const TopTools_IndexedMapOfShape& shape_map,
                                      const ShapeEntityMap& shape_entity_map,
                                      const GeometryEntityImplPtr& root_entity) {
    // Find the root shape's index and connect it to part
    const int root_index = shape_map.FindIndex(root_shape);
    if(root_index > 0) {
        auto it = shape_entity_map.find(root_index);
        if(it != shape_entity_map.end()) {
            (void)m_document->addChildEdge(*root_entity, *it->second);
            // Now recursively build child relationships starting from root shape
            buildChildRelationships(root_shape, it->second, shape_map, shape_entity_map);
            return;
        }
    }

    // Root shape itself might not be in map (e.g., if it's the Part shape)
    // In this case, process direct children of root shape
    for(TopoDS_Iterator it(root_shape); it.More(); it.Next()) {
        const TopoDS_Shape& child_shape = it.Value();
        const int child_index = shape_map.FindIndex(child_shape);

        if(child_index <= 0) {
            continue;
        }

        auto entity_it = shape_entity_map.find(child_index);
        if(entity_it == shape_entity_map.end()) {
            continue;
        }

        (void)m_document->addChildEdge(*root_entity, *entity_it->second);
        buildChildRelationships(child_shape, entity_it->second, shape_map, shape_entity_map);
    }
}

void ShapeBuilder::buildChildRelationships(const TopoDS_Shape& parent_shape, // NOLINT
                                           const GeometryEntityImplPtr& parent_entity,
                                           const TopTools_IndexedMapOfShape& shape_map,
                                           const ShapeEntityMap& shape_entity_map) {
    // Iterate through direct children of this shape
    for(TopoDS_Iterator it(parent_shape); it.More(); it.Next()) {
        const TopoDS_Shape& child_shape = it.Value();
        const int child_index = shape_map.FindIndex(child_shape);

        if(child_index <= 0) {
            continue;
        }

        auto entity_it = shape_entity_map.find(child_index);
        if(entity_it == shape_entity_map.end()) {
            continue;
        }

        // Add parent-child relationship
        // Note: This may add multiple parents to the same child (shared edges/vertices)
        (void)m_document->addChildEdge(*parent_entity, *entity_it->second);

        // Recursively process children
        buildChildRelationships(child_shape, entity_it->second, shape_map, shape_entity_map);
    }
}

void ShapeBuilder::updateEntityCounts(const TopoDS_Shape& shape, ShapeBuildResult& result) {
    if(shape.IsNull()) {
        LOG_ERROR("Cannot update entity counts for null shape");
        return;
    }

    switch(shape.ShapeType()) {
    case TopAbs_VERTEX:
        ++result.m_vertexCount;
        break;
    case TopAbs_EDGE:
        ++result.m_edgeCount;
        break;
    case TopAbs_WIRE:
        ++result.m_wireCount;
        break;
    case TopAbs_FACE:
        ++result.m_faceCount;
        break;
    case TopAbs_SHELL:
        ++result.m_shellCount;
        break;
    case TopAbs_SOLID:
        ++result.m_solidCount;
        break;
    case TopAbs_COMPSOLID:
        // CompSolid not counted separately
        break;
    case TopAbs_COMPOUND:
        ++result.m_compoundCount;
        break;
    default:
        break;
    }
}

GeometryEntityImplPtr ShapeBuilder::createEntityForShape(const TopoDS_Shape& shape) {
    if(shape.IsNull()) {
        LOG_ERROR("Cannot create entity for null shape");
        return nullptr;
    }

    switch(shape.ShapeType()) {
    case TopAbs_VERTEX:
        return std::make_shared<VertexEntity>(TopoDS::Vertex(shape));
    case TopAbs_EDGE:
        return std::make_shared<EdgeEntity>(TopoDS::Edge(shape));
    case TopAbs_WIRE:
        return std::make_shared<WireEntity>(TopoDS::Wire(shape));
    case TopAbs_FACE:
        return std::make_shared<FaceEntity>(TopoDS::Face(shape));
    case TopAbs_SHELL:
        return std::make_shared<ShellEntity>(TopoDS::Shell(shape));
    case TopAbs_SOLID:
        return std::make_shared<SolidEntity>(TopoDS::Solid(shape));
    case TopAbs_COMPSOLID:
        return std::make_shared<CompSolidEntity>(TopoDS::CompSolid(shape));
    case TopAbs_COMPOUND:
        return std::make_shared<CompoundEntity>(TopoDS::Compound(shape));
    default:
        LOG_ERROR("Unsupported shape type for entity creation");
        return nullptr;
    }
}

} // namespace OpenGeoLab::Geometry