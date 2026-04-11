/**
 * @file topology_utils.cpp
 * @brief OCC topology extraction implementations
 */

#include <opengeolab/geometry/topology_utils.hpp>

#include <opengeolab/geometry/shape_entry.hpp>

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <BRepGProp_Face.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>

#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_SurfaceType.hxx>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>

namespace OpenGeoLab::Geometry {

namespace {

std::string_view surfaceTypeName(GeomAbs_SurfaceType type) {
    switch(type) {
    case GeomAbs_Plane:
        return "plane";
    case GeomAbs_Cylinder:
        return "cylinder";
    case GeomAbs_Cone:
        return "cone";
    case GeomAbs_Sphere:
        return "sphere";
    case GeomAbs_Torus:
        return "torus";
    case GeomAbs_BSplineSurface:
        return "bspline";
    default:
        return "other";
    }
}

std::string_view curveTypeName(GeomAbs_CurveType type) {
    switch(type) {
    case GeomAbs_Line:
        return "line";
    case GeomAbs_Circle:
        return "circle";
    case GeomAbs_Ellipse:
        return "ellipse";
    case GeomAbs_Parabola:
        return "parabola";
    case GeomAbs_Hyperbola:
        return "hyperbola";
    case GeomAbs_BSplineCurve:
        return "bspline";
    default:
        return "other";
    }
}

std::array<double, 3> toArray(const gp_Pnt& p) { return {p.X(), p.Y(), p.Z()}; }

std::array<double, 3> toArray(const gp_Dir& d) { return {d.X(), d.Y(), d.Z()}; }

} // anonymous namespace

// ── Extraction ───────────────────────────────────────────────

FaceInfo extractFaceInfo(uint32_t local_id, const TopoDS_Face& face) {
    FaceInfo info;
    info.localId = local_id;

    BRepAdaptor_Surface adaptor(face);
    info.surfaceType = std::string(surfaceTypeName(adaptor.GetType()));

    // Area
    GProp_GProps props;
    BRepGProp::SurfaceProperties(face, props);
    info.area = props.Mass();

    // Center and normal at parametric midpoint
    BRepGProp_Face face_props(face);
    Standard_Real u1 = 0;
    Standard_Real u2 = 0;
    Standard_Real v1 = 0;
    Standard_Real v2 = 0;
    face_props.Bounds(u1, u2, v1, v2);
    gp_Pnt center;
    gp_Vec normal;
    face_props.Normal((u1 + u2) / 2.0, (v1 + v2) / 2.0, center, normal);
    info.center = toArray(center);

    constexpr double kEpsilon = 1e-10;
    if(normal.Magnitude() > kEpsilon) {
        normal.Normalize();
        info.normal = {normal.X(), normal.Y(), normal.Z()};
    }

    // Type-specific axis/radius
    switch(adaptor.GetType()) {
    case GeomAbs_Cylinder: {
        auto cyl = adaptor.Cylinder();
        info.axis = toArray(cyl.Axis().Direction());
        info.radius = cyl.Radius();
        break;
    }
    case GeomAbs_Cone: {
        auto cone = adaptor.Cone();
        info.axis = toArray(cone.Axis().Direction());
        info.radius = cone.RefRadius();
        break;
    }
    case GeomAbs_Sphere: {
        auto sph = adaptor.Sphere();
        info.radius = sph.Radius();
        break;
    }
    case GeomAbs_Torus: {
        auto tor = adaptor.Torus();
        info.axis = toArray(tor.Axis().Direction());
        info.radius = tor.MajorRadius();
        break;
    }
    default:
        break;
    }

    return info;
}

EdgeInfo extractEdgeInfo(uint32_t local_id, const TopoDS_Edge& edge) {
    EdgeInfo info;
    info.localId = local_id;

    BRepAdaptor_Curve adaptor(edge);
    info.curveType = std::string(curveTypeName(adaptor.GetType()));

    // Length
    GProp_GProps props;
    BRepGProp::LinearProperties(edge, props);
    info.length = props.Mass();

    // Endpoints
    gp_Pnt start_pt;
    gp_Pnt end_pt;
    adaptor.D0(adaptor.FirstParameter(), start_pt);
    adaptor.D0(adaptor.LastParameter(), end_pt);
    info.start = toArray(start_pt);
    info.end = toArray(end_pt);

    // Type-specific center/radius
    switch(adaptor.GetType()) {
    case GeomAbs_Circle: {
        auto circle = adaptor.Circle();
        info.center = toArray(circle.Location());
        info.radius = circle.Radius();
        break;
    }
    case GeomAbs_Ellipse: {
        auto ellipse = adaptor.Ellipse();
        info.center = toArray(ellipse.Location());
        info.radius = ellipse.MajorRadius();
        break;
    }
    default:
        break;
    }

    return info;
}

VertexInfo extractVertexInfo(uint32_t local_id, const TopoDS_Vertex& vertex) {
    return {local_id, toArray(BRep_Tool::Pnt(vertex))};
}

// ── JSON Serialisation ───────────────────────────────────────

nlohmann::json toJson(const FaceInfo& info) {
    nlohmann::json j = {{"localId", info.localId},
                        {"surfaceType", info.surfaceType},
                        {"center", info.center},
                        {"normal", info.normal},
                        {"area", info.area}};
    if(info.axis) {
        j["axis"] = *info.axis;
    }
    if(info.radius) {
        j["radius"] = *info.radius;
    }
    return j;
}

nlohmann::json toJson(const EdgeInfo& info) {
    nlohmann::json j = {{"localId", info.localId},
                        {"curveType", info.curveType},
                        {"start", info.start},
                        {"end", info.end},
                        {"length", info.length}};
    if(info.center) {
        j["center"] = *info.center;
    }
    if(info.radius) {
        j["radius"] = *info.radius;
    }
    return j;
}

nlohmann::json toJson(const VertexInfo& info) {
    return {{"localId", info.localId}, {"position", info.position}};
}

// ── Adjacency ────────────────────────────────────────────────

std::unordered_map<uint32_t, std::vector<uint32_t>>
buildEdgeToFaceAdjacency(const ShapeEntry& entry) {
    TopTools_IndexedDataMapOfShapeListOfShape map;
    TopExp::MapShapesAndAncestors(entry.shape, TopAbs_EDGE, TopAbs_FACE, map);

    std::unordered_map<uint32_t, std::vector<uint32_t>> result;
    for(int i = 1; i <= entry.edgeMap.Extent(); ++i) {
        const auto& edge = entry.edgeMap(i);
        if(!map.Contains(edge)) {
            continue;
        }
        const auto& faces = map.FindFromKey(edge);
        std::vector<uint32_t> ids;
        for(TopTools_ListIteratorOfListOfShape it(faces); it.More(); it.Next()) {
            int idx = entry.faceMap.FindIndex(it.Value());
            if(idx > 0) {
                ids.push_back(static_cast<uint32_t>(idx));
            }
        }
        std::sort(ids.begin(), ids.end());
        ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
        if(!ids.empty()) {
            result[static_cast<uint32_t>(i)] = std::move(ids);
        }
    }
    return result;
}

std::unordered_map<uint32_t, std::vector<uint32_t>>
buildVertexToEdgeAdjacency(const ShapeEntry& entry) {
    TopTools_IndexedDataMapOfShapeListOfShape map;
    TopExp::MapShapesAndAncestors(entry.shape, TopAbs_VERTEX, TopAbs_EDGE, map);

    std::unordered_map<uint32_t, std::vector<uint32_t>> result;
    for(int i = 1; i <= entry.vertexMap.Extent(); ++i) {
        const auto& vtx = entry.vertexMap(i);
        if(!map.Contains(vtx)) {
            continue;
        }
        const auto& edges = map.FindFromKey(vtx);
        std::vector<uint32_t> ids;
        for(TopTools_ListIteratorOfListOfShape it(edges); it.More(); it.Next()) {
            int idx = entry.edgeMap.FindIndex(it.Value());
            if(idx > 0) {
                ids.push_back(static_cast<uint32_t>(idx));
            }
        }
        std::sort(ids.begin(), ids.end());
        ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
        if(!ids.empty()) {
            result[static_cast<uint32_t>(i)] = std::move(ids);
        }
    }
    return result;
}

std::unordered_map<uint32_t, std::vector<uint32_t>>
buildFaceToEdgeAdjacency(const ShapeEntry& entry) {
    std::unordered_map<uint32_t, std::vector<uint32_t>> result;
    for(int i = 1; i <= entry.faceMap.Extent(); ++i) {
        const auto& face = entry.faceMap(i);
        std::vector<uint32_t> edge_ids;
        for(TopExp_Explorer exp(face, TopAbs_EDGE); exp.More(); exp.Next()) {
            int idx = entry.edgeMap.FindIndex(exp.Current());
            if(idx > 0) {
                edge_ids.push_back(static_cast<uint32_t>(idx));
            }
        }
        std::sort(edge_ids.begin(), edge_ids.end());
        edge_ids.erase(std::unique(edge_ids.begin(), edge_ids.end()), edge_ids.end());
        if(!edge_ids.empty()) {
            result[static_cast<uint32_t>(i)] = std::move(edge_ids);
        }
    }
    return result;
}

// ── Bounding Box ─────────────────────────────────────────────

std::optional<std::pair<std::array<double, 3>, std::array<double, 3>>>
computeSubShapeBounds(const TopoDS_Shape& sub_shape) {
    Bnd_Box box;
    BRepBndLib::Add(sub_shape, box);
    if(box.IsVoid()) {
        return std::nullopt;
    }
    Standard_Real x_min = 0;
    Standard_Real y_min = 0;
    Standard_Real z_min = 0;
    Standard_Real x_max = 0;
    Standard_Real y_max = 0;
    Standard_Real z_max = 0;
    box.Get(x_min, y_min, z_min, x_max, y_max, z_max);
    return std::pair{std::array{x_min, y_min, z_min}, std::array{x_max, y_max, z_max}};
}

} // namespace OpenGeoLab::Geometry
