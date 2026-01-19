/**
 * @file tessellator.cpp
 * @brief Implementation of geometry tessellation
 */

#include "render/tessellator.hpp"
#include "util/logger.hpp"

#include <BRepAdaptor_Curve.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_UniformDeflection.hxx>
#include <Poly_Triangulation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>


#include <cmath>
#include <limits>

namespace OpenGeoLab::Render {

Tessellator::Tessellator() : m_settings(TessellationSettings::Medium()) {}

Tessellator::Tessellator(const TessellationSettings& settings) : m_settings(settings) {}

std::vector<RenderMeshPtr>
Tessellator::tessellatePart(const std::shared_ptr<Geometry::PartEntity>& part) {

    std::vector<RenderMeshPtr> result;

    if(!part || !part->hasValidShape()) {
        LOG_WARN("Cannot tessellate null or invalid part");
        return result;
    }

    LOG_DEBUG("Tessellating part: {}", part->name());

    const TopoDS_Shape& shape = part->shape();
    ensureTriangulation(shape);

    // Tessellate each face separately for picking support
    for(TopExp_Explorer faceExp(shape, TopAbs_FACE); faceExp.More(); faceExp.Next()) {
        const TopoDS_Face& face = TopoDS::Face(faceExp.Current());

        auto mesh = std::make_shared<RenderMesh>();
        mesh->entityId = Geometry::generateEntityId();
        mesh->entityType = Geometry::EntityType::Face;
        mesh->baseColor = part->color();

        extractFaceData(face, *mesh);
        extractEdgeData(face, *mesh);
        calculateBoundingBox(*mesh);

        if(mesh->hasFaces() || mesh->hasEdges()) {
            result.push_back(mesh);
        }
    }

    LOG_DEBUG("Tessellated {} faces from part {}", result.size(), part->name());
    return result;
}

RenderMeshPtr Tessellator::tessellateFace(const std::shared_ptr<Geometry::FaceEntity>& face) {
    auto mesh = std::make_shared<RenderMesh>();

    if(!face || !face->hasValidShape()) {
        return mesh;
    }

    mesh->entityId = face->id();
    mesh->entityType = Geometry::EntityType::Face;
    mesh->baseColor = face->color();
    mesh->visible = face->isVisible();
    mesh->selected = face->isSelected();

    const TopoDS_Shape& shape = face->shape();
    ensureTriangulation(shape);
    extractFaceData(shape, *mesh);
    extractEdgeData(shape, *mesh);
    calculateBoundingBox(*mesh);

    return mesh;
}

RenderMeshPtr Tessellator::tessellateEdge(const std::shared_ptr<Geometry::EdgeEntity>& edge) {
    auto mesh = std::make_shared<RenderMesh>();

    if(!edge || !edge->hasValidShape()) {
        return mesh;
    }

    mesh->entityId = edge->id();
    mesh->entityType = Geometry::EntityType::Edge;
    mesh->baseColor = edge->color();
    mesh->visible = edge->isVisible();
    mesh->selected = edge->isSelected();

    extractEdgeData(edge->shape(), *mesh);
    calculateBoundingBox(*mesh);

    return mesh;
}

RenderMeshPtr Tessellator::tessellateSolid(const std::shared_ptr<Geometry::SolidEntity>& solid) {
    auto mesh = std::make_shared<RenderMesh>();

    if(!solid || !solid->hasValidShape()) {
        return mesh;
    }

    mesh->entityId = solid->id();
    mesh->entityType = Geometry::EntityType::Solid;
    mesh->baseColor = solid->color();
    mesh->visible = solid->isVisible();
    mesh->selected = solid->isSelected();

    const TopoDS_Shape& shape = solid->shape();
    ensureTriangulation(shape);
    extractFaceData(shape, *mesh);
    extractEdgeData(shape, *mesh);
    calculateBoundingBox(*mesh);

    return mesh;
}

RenderMeshPtr Tessellator::tessellateShape(const TopoDS_Shape& shape, Geometry::EntityId entityId) {
    auto mesh = std::make_shared<RenderMesh>();

    if(shape.IsNull()) {
        return mesh;
    }

    mesh->entityId = entityId;
    mesh->entityType = Geometry::EntityType::Compound;

    ensureTriangulation(shape);
    extractFaceData(shape, *mesh);
    extractEdgeData(shape, *mesh);
    calculateBoundingBox(*mesh);

    return mesh;
}

void Tessellator::ensureTriangulation(const TopoDS_Shape& shape) {
    // Create incremental mesh if not already triangulated
    BRepMesh_IncrementalMesh mesher(shape, m_settings.linearDeflection,
                                    m_settings.relativeDeflection, m_settings.angularDeflection);
    mesher.Perform();

    if(!mesher.IsDone()) {
        LOG_WARN("Mesh generation did not complete successfully");
    }
}

void Tessellator::extractFaceData(const TopoDS_Shape& shape, RenderMesh& mesh) {
    for(TopExp_Explorer faceExp(shape, TopAbs_FACE); faceExp.More(); faceExp.Next()) {
        const TopoDS_Face& face = TopoDS::Face(faceExp.Current());
        TopLoc_Location location;

        Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);
        if(triangulation.IsNull()) {
            continue;
        }

        const gp_Trsf& transform = location.Transformation();
        bool reversed = (face.Orientation() == TopAbs_REVERSED);

        // Get existing vertex count for index offset
        uint32_t baseIndex = static_cast<uint32_t>(mesh.vertices.size());

        // Add vertices
        int nbNodes = triangulation->NbNodes();
        for(int i = 1; i <= nbNodes; ++i) {
            gp_Pnt point = triangulation->Node(i).Transformed(transform);

            RenderVertex vertex;
            vertex.position = {static_cast<float>(point.X()), static_cast<float>(point.Y()),
                               static_cast<float>(point.Z())};

            // Get normal if available
            if(triangulation->HasNormals()) {
                gp_Dir normal = triangulation->Normal(i);
                if(reversed) {
                    normal.Reverse();
                }
                vertex.normal = {static_cast<float>(normal.X()), static_cast<float>(normal.Y()),
                                 static_cast<float>(normal.Z())};
            } else {
                vertex.normal = {0.0f, 0.0f, 1.0f};
            }

            vertex.color = {mesh.baseColor.r, mesh.baseColor.g, mesh.baseColor.b, mesh.baseColor.a};
            vertex.entityId = static_cast<uint32_t>(mesh.entityId);

            mesh.vertices.push_back(vertex);
        }

        // Add triangles
        int nbTriangles = triangulation->NbTriangles();
        for(int i = 1; i <= nbTriangles; ++i) {
            Poly_Triangle triangle = triangulation->Triangle(i);
            int n1 = 0;
            int n2 = 0;
            int n3 = 0;
            triangle.Get(n1, n2, n3);

            if(reversed) {
                std::swap(n2, n3);
            }

            mesh.faceIndices.push_back(baseIndex + static_cast<uint32_t>(n1 - 1));
            mesh.faceIndices.push_back(baseIndex + static_cast<uint32_t>(n2 - 1));
            mesh.faceIndices.push_back(baseIndex + static_cast<uint32_t>(n3 - 1));
        }
    }
}

void Tessellator::extractEdgeData(const TopoDS_Shape& shape, RenderMesh& mesh) {
    for(TopExp_Explorer edgeExp(shape, TopAbs_EDGE); edgeExp.More(); edgeExp.Next()) {
        const TopoDS_Edge& edge = TopoDS::Edge(edgeExp.Current());

        if(BRep_Tool::Degenerated(edge)) {
            continue;
        }

        BRepAdaptor_Curve curve(edge);
        double first = curve.FirstParameter();
        double last = curve.LastParameter();

        if(std::abs(last - first) < Precision::Confusion()) {
            continue;
        }

        // Discretize the curve
        GCPnts_UniformDeflection discretizer(curve, m_settings.linearDeflection);
        if(!discretizer.IsDone() || discretizer.NbPoints() < 2) {
            continue;
        }

        uint32_t baseIndex = static_cast<uint32_t>(mesh.edgeVertices.size());

        // Add edge vertices
        for(int i = 1; i <= discretizer.NbPoints(); ++i) {
            gp_Pnt point = discretizer.Value(i);

            EdgeVertex vertex;
            vertex.position = {static_cast<float>(point.X()), static_cast<float>(point.Y()),
                               static_cast<float>(point.Z())};
            vertex.color = {0.0f, 0.0f, 0.0f, 1.0f}; // Black edges
            vertex.entityId = static_cast<uint32_t>(mesh.entityId);

            mesh.edgeVertices.push_back(vertex);
        }

        // Add line segments
        for(int i = 1; i < discretizer.NbPoints(); ++i) {
            mesh.edgeIndices.push_back(baseIndex + static_cast<uint32_t>(i - 1));
            mesh.edgeIndices.push_back(baseIndex + static_cast<uint32_t>(i));
        }
    }
}

void Tessellator::calculateBoundingBox(RenderMesh& mesh) {
    if(mesh.vertices.empty() && mesh.edgeVertices.empty()) {
        return;
    }

    constexpr float maxFloat = std::numeric_limits<float>::max();
    constexpr float minFloat = std::numeric_limits<float>::lowest();

    float minX = maxFloat, minY = maxFloat, minZ = maxFloat;
    float maxX = minFloat, maxY = minFloat, maxZ = minFloat;

    for(const auto& vertex : mesh.vertices) {
        minX = std::min(minX, vertex.position[0]);
        minY = std::min(minY, vertex.position[1]);
        minZ = std::min(minZ, vertex.position[2]);
        maxX = std::max(maxX, vertex.position[0]);
        maxY = std::max(maxY, vertex.position[1]);
        maxZ = std::max(maxZ, vertex.position[2]);
    }

    for(const auto& vertex : mesh.edgeVertices) {
        minX = std::min(minX, vertex.position[0]);
        minY = std::min(minY, vertex.position[1]);
        minZ = std::min(minZ, vertex.position[2]);
        maxX = std::max(maxX, vertex.position[0]);
        maxY = std::max(maxY, vertex.position[1]);
        maxZ = std::max(maxZ, vertex.position[2]);
    }

    mesh.boundingBox.min = Geometry::Point3D(minX, minY, minZ);
    mesh.boundingBox.max = Geometry::Point3D(maxX, maxY, maxZ);
}

} // namespace OpenGeoLab::Render
