/**
 * @file shape_builder.cpp
 * @brief Implementation of ShapeBuilder for entity hierarchy construction
 */

#include "geometry/shape_builder.hpp"
#include "geometry/comp_solid_entity.hpp"
#include "geometry/compound_entity.hpp"
#include "geometry/edge_entity.hpp"
#include "geometry/face_entity.hpp"
#include "geometry/shell_entity.hpp"
#include "geometry/solid_entity.hpp"
#include "geometry/vertex_entity.hpp"
#include "geometry/wire_entity.hpp"
#include "util/logger.hpp"

#include <BRepAdaptor_Curve.hxx>
#include <BRepGProp.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_TangentialDeflection.hxx>
#include <GProp_GProps.hxx>
#include <Geom_Surface.hxx>
#include <Poly_Triangulation.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
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

// =============================================================================
// ShapeBuilder Implementation
// =============================================================================

ShapeBuilder::ShapeBuilder(GeometryDocumentPtr document) : m_document(std::move(document)) {
    if(!m_document) {
        throw std::invalid_argument("ShapeBuilder requires a valid document");
    }
}

ShapeBuildResult ShapeBuilder::buildFromShape(const TopoDS_Shape& shape,
                                              const std::string& part_name,
                                              const ShapeBuildOptions& options,
                                              BuildProgressCallback progress_callback) {
    if(shape.IsNull()) {
        return ShapeBuildResult::failure("Input shape is null");
    }

    // Report start
    if(progress_callback && !progress_callback(0.0, "Creating part entity...")) {
        return ShapeBuildResult::failure("Operation cancelled");
    }

    // Create the root part entity
    auto part = std::make_shared<PartEntity>(shape);
    part->setName(part_name);

    // Add part to document
    if(!m_document->addEntity(part)) {
        return ShapeBuildResult::failure("Failed to add part to document");
    }

    ShapeBuildResult result = ShapeBuildResult::success(part);

    // Report progress
    if(progress_callback && !progress_callback(0.05, "Building entity hierarchy...")) {
        // Rollback - remove the part
        (void)m_document->removeEntity(part->entityId());
        return ShapeBuildResult::failure("Operation cancelled");
    }

    // Determine the root shape type
    const TopAbs_ShapeEnum root_shape_type = shape.ShapeType();

    // For non-compound shapes, we need to create an intermediate entity for the root shape type
    // Part -> RootEntity (Solid/Shell/etc.) -> SubEntities
    GeometryEntityPtr hierarchy_root = part;

    if(root_shape_type != TopAbs_COMPOUND && root_shape_type != TopAbs_COMPSOLID) {
        // Create entity for the root shape itself (not just sub-shapes)
        auto root_entity = createEntityForShape(shape);
        if(root_entity) {
            if(m_document->addEntity(root_entity)) {
                if(part->addChild(root_entity)) {
                    // Update statistics for root entity
                    switch(root_shape_type) {
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
                    default:
                        break;
                    }
                    hierarchy_root = root_entity;
                } else {
                    // Failed to add as child, use part as root
                    (void)m_document->removeEntity(root_entity->entityId());
                    LOG_WARN("Failed to add root entity as child of part");
                }
            }
        }
    }

    // Build the sub-shape hierarchy starting from the hierarchy_root
    // Progress: 0.05 to 0.70 for building
    buildSubShapes(shape, hierarchy_root, options, result, progress_callback, 0.05, 0.65);

    // Generate render data if requested
    if(options.m_generateRenderData) {
        if(progress_callback && !progress_callback(0.70, "Generating render data...")) {
            return ShapeBuildResult::failure("Operation cancelled");
        }
        result.m_renderData = generateRenderData(part, options, m_partCounter);
    }

    // Generate mesh metadata if requested
    if(options.m_generateMeshMetadata) {
        if(progress_callback && !progress_callback(0.85, "Generating mesh metadata...")) {
            return ShapeBuildResult::failure("Operation cancelled");
        }
        result.m_meshMetadata = generateMeshMetadata(part);
    }

    // Increment part counter for color generation
    ++m_partCounter;

    // Final progress report
    if(progress_callback) {
        progress_callback(1.0, "Build complete");
    }

    LOG_DEBUG("Built part '{}' with {} entities", part_name, result.totalEntityCount());

    return result;
}

void ShapeBuilder::buildSubShapes(const TopoDS_Shape& shape,
                                  const GeometryEntityPtr& parent,
                                  const ShapeBuildOptions& options,
                                  ShapeBuildResult& result,
                                  BuildProgressCallback& progress_callback,
                                  double progress_base,
                                  double progress_scale) {
    // Iterate over immediate sub-shapes
    size_t child_index = 0;
    size_t total_children = 0;

    // Count children first for progress reporting
    for(TopoDS_Iterator it(shape); it.More(); it.Next()) {
        ++total_children;
    }

    for(TopoDS_Iterator it(shape); it.More(); it.Next(), ++child_index) {
        const TopoDS_Shape& child_shape = it.Value();
        if(child_shape.IsNull()) {
            continue;
        }

        // Check if we should build this shape type
        const TopAbs_ShapeEnum shape_type = child_shape.ShapeType();
        bool should_build = true;

        switch(shape_type) {
        case TopAbs_VERTEX:
            should_build = options.m_buildVertices;
            break;
        case TopAbs_EDGE:
            should_build = options.m_buildEdges;
            break;
        case TopAbs_WIRE:
            should_build = options.m_buildWires;
            break;
        case TopAbs_FACE:
            should_build = options.m_buildFaces;
            break;
        case TopAbs_SHELL:
            should_build = options.m_buildShells;
            break;
        case TopAbs_SOLID:
            should_build = options.m_buildSolids;
            break;
        case TopAbs_COMPSOLID:
        case TopAbs_COMPOUND:
            should_build = options.m_buildCompounds;
            break;
        default:
            should_build = false;
            break;
        }

        if(!should_build) {
            continue;
        }

        // Create entity for this shape
        auto child_entity = createEntityForShape(child_shape);
        if(!child_entity) {
            continue;
        }

        // Add to document
        if(!m_document->addEntity(child_entity)) {
            LOG_WARN("Failed to add entity to document");
            continue;
        }

        // Establish parent-child relationship
        if(!parent->addChild(child_entity)) {
            LOG_WARN("Failed to add parent-child relationship");
        }

        // Update statistics
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
        case TopAbs_COMPSOLID:
        case TopAbs_COMPOUND:
            ++result.m_compoundCount;
            break;
        default:
            break;
        }

        // Report progress
        if(progress_callback && total_children > 0) {
            double child_progress =
                progress_base +
                (static_cast<double>(child_index + 1) / total_children) * progress_scale;
            if(!progress_callback(child_progress, "Building entities...")) {
                return; // Cancelled
            }
        }

        // Recursively build sub-shapes
        double sub_progress_base = progress_base + (static_cast<double>(child_index) /
                                                    std::max(total_children, size_t{1})) *
                                                       progress_scale;
        double sub_progress_scale = progress_scale / std::max(total_children, size_t{1});

        buildSubShapes(child_shape, child_entity, options, result, progress_callback,
                       sub_progress_base, sub_progress_scale);
    }
}

GeometryEntityPtr ShapeBuilder::createEntityForShape(const TopoDS_Shape& shape) {
    if(shape.IsNull()) {
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
        return nullptr;
    }
}

PartRenderDataPtr ShapeBuilder::generateRenderData(const PartEntityPtr& part,
                                                   const ShapeBuildOptions& options,
                                                   size_t part_index) {
    if(!part) {
        return nullptr;
    }

    auto render_data = std::make_shared<PartRenderData>();
    render_data->m_partEntityId = part->entityId();
    render_data->m_partName = part->name();
    render_data->m_baseColor = RenderColor::fromIndex(part_index);
    render_data->m_boundingBox = part->boundingBox();

    const TopoDS_Shape& shape = part->shape();

    // Mesh the shape for triangulation
    BRepMesh_IncrementalMesh mesher(shape, options.m_tessellation.m_linearDeflection,
                                    options.m_tessellation.m_relative,
                                    options.m_tessellation.m_angularDeflection);
    mesher.Perform();

    // Extract face triangulations
    for(TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        TopoDS_Face face = TopoDS::Face(exp.Current());
        RenderFace render_face =
            tessellateFace(face, options.m_tessellation, render_data->m_baseColor);

        // Find the face entity ID if it exists
        if(auto face_entity = m_document->findByShape(face)) {
            render_face.m_entityId = face_entity->entityId();
        }

        if(!render_face.m_indices.empty()) {
            render_data->m_faces.push_back(std::move(render_face));
        }
    }

    // Extract edge discretizations
    for(TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(exp.Current());
        RenderEdge render_edge = discretizeEdge(edge, options.m_tessellation);

        // Find the edge entity ID if it exists
        if(auto edge_entity = m_document->findByShape(edge)) {
            render_edge.m_entityId = edge_entity->entityId();
        }

        if(!render_edge.m_points.empty()) {
            render_data->m_edges.push_back(std::move(render_edge));
        }
    }

    LOG_DEBUG("Generated render data: {} faces, {} edges", render_data->m_faces.size(),
              render_data->m_edges.size());

    return render_data;
}

PartMeshMetadataPtr ShapeBuilder::generateMeshMetadata(const PartEntityPtr& part) {
    if(!part) {
        return nullptr;
    }

    auto metadata = std::make_shared<PartMeshMetadata>();
    metadata->m_partEntityId = part->entityId();
    metadata->m_partName = part->name();
    metadata->m_boundingBox = part->boundingBox();
    metadata->m_characteristicLength = metadata->m_boundingBox.diagonal();

    const TopoDS_Shape& shape = part->shape();

    // Extract solid metadata
    for(TopExp_Explorer exp(shape, TopAbs_SOLID); exp.More(); exp.Next()) {
        TopoDS_Solid solid = TopoDS::Solid(exp.Current());
        SolidMetadata solid_meta;

        if(auto solid_entity = m_document->findByShape(solid)) {
            solid_meta.m_entityId = solid_entity->entityId();
        }

        // Compute volume and surface area
        GProp_GProps vol_props;
        BRepGProp::VolumeProperties(solid, vol_props);
        solid_meta.m_volume = vol_props.Mass();
        gp_Pnt center = vol_props.CentreOfMass();
        solid_meta.m_centerOfMass = Point3D(center.X(), center.Y(), center.Z());

        GProp_GProps surf_props;
        BRepGProp::SurfaceProperties(solid, surf_props);
        solid_meta.m_surfaceArea = surf_props.Mass();

        // Count sub-shapes
        TopTools_IndexedMapOfShape faces, edges, vertices;
        TopExp::MapShapes(solid, TopAbs_FACE, faces);
        TopExp::MapShapes(solid, TopAbs_EDGE, edges);
        TopExp::MapShapes(solid, TopAbs_VERTEX, vertices);
        solid_meta.m_faceCount = faces.Extent();
        solid_meta.m_edgeCount = edges.Extent();
        solid_meta.m_vertexCount = vertices.Extent();

        metadata->m_solids.push_back(std::move(solid_meta));
    }

    // Extract face metadata
    for(TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        TopoDS_Face face = TopoDS::Face(exp.Current());
        FaceMetadata face_meta;

        if(auto face_entity = m_document->findByShape(face)) {
            face_meta.m_entityId = face_entity->entityId();
        }

        // Get surface type
        Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
        if(!surface.IsNull()) {
            // Simplified surface type detection
            face_meta.m_surfaceType = SurfaceType::BSpline; // Default, could be enhanced
        }

        // Compute area
        GProp_GProps props;
        BRepGProp::SurfaceProperties(face, props);
        face_meta.m_area = props.Mass();

        face_meta.m_isForward = (face.Orientation() == TopAbs_FORWARD);

        metadata->m_faces.push_back(std::move(face_meta));
    }

    // Extract edge metadata
    for(TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(exp.Current());
        EdgeMetadata edge_meta;

        if(auto edge_entity = m_document->findByShape(edge)) {
            edge_meta.m_entityId = edge_entity->entityId();
        }

        // Compute length
        GProp_GProps props;
        BRepGProp::LinearProperties(edge, props);
        edge_meta.m_length = props.Mass();

        edge_meta.m_isDegenerate = BRep_Tool::Degenerated(edge);

        // Get endpoints
        TopoDS_Vertex v1, v2;
        TopExp::Vertices(edge, v1, v2);
        if(!v1.IsNull()) {
            gp_Pnt p = BRep_Tool::Pnt(v1);
            edge_meta.m_startPoint = Point3D(p.X(), p.Y(), p.Z());
        }
        if(!v2.IsNull()) {
            gp_Pnt p = BRep_Tool::Pnt(v2);
            edge_meta.m_endPoint = Point3D(p.X(), p.Y(), p.Z());
        }

        metadata->m_edges.push_back(std::move(edge_meta));
    }

    LOG_DEBUG("Generated mesh metadata: {} solids, {} faces, {} edges", metadata->m_solids.size(),
              metadata->m_faces.size(), metadata->m_edges.size());

    return metadata;
}

RenderFace ShapeBuilder::tessellateFace(const TopoDS_Face& face,
                                        const TessellationParams& /*params*/,
                                        const RenderColor& color) {
    RenderFace render_face;

    TopLoc_Location location;
    Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);

    if(triangulation.IsNull()) {
        return render_face;
    }

    const gp_Trsf& trsf = location.Transformation();
    const bool has_transform = !location.IsIdentity();
    const bool face_reversed = (face.Orientation() == TopAbs_REVERSED);

    // Copy nodes
    const int num_nodes = triangulation->NbNodes();
    render_face.m_vertices.reserve(num_nodes);

    for(int i = 1; i <= num_nodes; ++i) {
        gp_Pnt node = triangulation->Node(i);
        if(has_transform) {
            node.Transform(trsf);
        }

        // Get normal if available
        gp_Dir normal(0, 0, 1);
        if(triangulation->HasNormals()) {
            normal = triangulation->Normal(i);
            if(has_transform) {
                normal.Transform(trsf);
            }
            if(face_reversed) {
                normal.Reverse();
            }
        }

        RenderVertex vertex(static_cast<float>(node.X()), static_cast<float>(node.Y()),
                            static_cast<float>(node.Z()), static_cast<float>(normal.X()),
                            static_cast<float>(normal.Y()), static_cast<float>(normal.Z()));
        vertex.setColor(color);
        render_face.m_vertices.push_back(vertex);
    }

    // Copy triangles
    const int num_triangles = triangulation->NbTriangles();
    render_face.m_indices.reserve(num_triangles * 3);

    for(int i = 1; i <= num_triangles; ++i) {
        const Poly_Triangle& tri = triangulation->Triangle(i);
        int n1, n2, n3;
        tri.Get(n1, n2, n3);

        // Convert to 0-based indices
        if(face_reversed) {
            // Reverse winding order for reversed faces
            render_face.m_indices.push_back(static_cast<uint32_t>(n1 - 1));
            render_face.m_indices.push_back(static_cast<uint32_t>(n3 - 1));
            render_face.m_indices.push_back(static_cast<uint32_t>(n2 - 1));
        } else {
            render_face.m_indices.push_back(static_cast<uint32_t>(n1 - 1));
            render_face.m_indices.push_back(static_cast<uint32_t>(n2 - 1));
            render_face.m_indices.push_back(static_cast<uint32_t>(n3 - 1));
        }
    }

    return render_face;
}

RenderEdge ShapeBuilder::discretizeEdge(const TopoDS_Edge& edge, const TessellationParams& params) {
    RenderEdge render_edge;

    if(BRep_Tool::Degenerated(edge)) {
        return render_edge;
    }

    try {
        BRepAdaptor_Curve curve(edge);
        GCPnts_TangentialDeflection discretizer(curve, params.m_angularDeflection,
                                                params.m_linearDeflection);

        const int num_points = discretizer.NbPoints();
        render_edge.m_points.reserve(num_points);

        for(int i = 1; i <= num_points; ++i) {
            gp_Pnt p = discretizer.Value(i);
            render_edge.m_points.emplace_back(p.X(), p.Y(), p.Z());
        }
    } catch(const Standard_Failure& /*e*/) {
        // Edge discretization failed, return empty
    }

    return render_edge;
}

} // namespace OpenGeoLab::Geometry
