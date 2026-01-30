#include "shape_builder.hpp"
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

namespace OpenGeoLab::Geometry {
ShapeBuilder::ShapeBuilder(GeometryDocumentPtr document) : m_document(std::move(document)) {
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

    auto part = std::make_shared<PartEntity>(shape);
    part->setName(part_name.empty() ? "Part" : part_name);

    if(!m_document->addEntity(part)) {
        return ShapeBuildResult::failure("Failed to add part entity to document");
    }
    ShapeBuildResult result = ShapeBuildResult::success(part);

    if(!progress_callback(0.1, "Building entity hierarchy...")) {
        (void)m_document->removeEntity(part->entityId());
        return ShapeBuildResult::failure("Operation cancelled");
    }

    const TopAbs_ShapeEnum root_shape_type = shape.ShapeType();

    GeometryEntityPtr hierarchy_root = part;

    // part -> container shape (same with part) -> sub-shapes
    auto container_entity = createEntityForShape(shape);
    if(!container_entity) {
        (void)m_document->removeEntity(part->entityId());
        return ShapeBuildResult::failure("Unsupported root shape type for entity creation");
    }

    if(!m_document->addEntity(container_entity)) {
        (void)m_document->removeEntity(part->entityId());
        return ShapeBuildResult::failure("Failed to add container entity to document");
    }

    if(!part->addChild(container_entity)) {
        (void)m_document->removeEntity(container_entity->entityId());
        (void)m_document->removeEntity(part->entityId());
        return ShapeBuildResult::failure("Failed to link container entity to part");
    }

    updateEntityCounts(shape, result);
    auto subcallback = Util::makeScaledProgressCallback(progress_callback, 0.1, 0.9);
    buildSubShapes(shape, container_entity, result, subcallback);

    if(!progress_callback(1.0, "Shape build completed.")) {
        (void)m_document->removeEntity(part->entityId());
    }
    return result;
}

void ShapeBuilder::buildSubShapes(const TopoDS_Shape& shape, // NOLINT
                                  const GeometryEntityPtr& parent,
                                  ShapeBuildResult& result,
                                  Util::ProgressCallback& progress_callback) {
    if(shape.IsNull()) {
        LOG_ERROR("Cannot build sub-shapes for null shape");
        return;
    }

    size_t child_index = 0, total_children = 0;

    for(TopoDS_Iterator it(shape); it.More(); it.Next()) {
        ++total_children;
    }

    for(TopoDS_Iterator it(shape); it.More(); it.Next(), ++child_index) {
        const TopoDS_Shape& child_shape = it.Value();
        if(child_shape.IsNull()) {
            LOG_WARN("Skipping null sub-shape in hierarchy...");
            continue;
        }

        auto child_entity = createEntityForShape(child_shape);
        if(!child_entity) {
            LOG_WARN("Skipping unsupported shape type in hierarchy");
            continue;
        }

        if(!m_document->addEntity(child_entity)) {
            LOG_ERROR("Failed to add child entity to document");
            continue;
        }

        if(!parent->addChild(child_entity)) {
            LOG_ERROR("Failed to link child entity to parent");
            (void)m_document->removeEntity(child_entity->entityId());
            continue;
        }

        if(total_children > 0 &&
           !progress_callback((static_cast<double>(child_index) / total_children),
                              "Processing sub-shapes...")) {
            // Cancellation requested not handled here to allow cleanup
        }

        updateEntityCounts(child_shape, result);
        auto subcallback = Util::makeScaledProgressCallback(
            progress_callback,
            static_cast<double>(child_index) / std::max(total_children, size_t{1}),
            1.0 / std::max(total_children, size_t{1}));
        buildSubShapes(child_shape, child_entity, result, subcallback);
    }
}

void ShapeBuilder::updateEntityCounts(const TopoDS_Shape& shape, ShapeBuildResult& result) {
    if(shape.IsNull()) {
        LOG_ERROR("Cannot update entity counts for null shape");
        return;
    }
    const TopAbs_ShapeEnum shape_type = shape.ShapeType();
    switch(shape_type) {
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
    case TopAbs_COMPOUND:
        ++result.m_compoundCount;
        break;
    default:
        break;
    }
}

GeometryEntityPtr ShapeBuilder::createEntityForShape(const TopoDS_Shape& shape) {
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