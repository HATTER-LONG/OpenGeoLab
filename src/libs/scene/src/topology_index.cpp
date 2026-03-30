#include <opengeolab/scene/topology_index.hpp>

#include <opengeolab/geometry/shape_entry.hpp>

#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Wire.hxx>

namespace OpenGeoLab::Scene {

namespace {

template <typename MapType> std::optional<uint32_t> lookupValue(const MapType& map, uint32_t key) {
    const auto iterator = map.find(key);
    if(iterator == map.end()) {
        return std::nullopt;
    }

    return iterator->second;
}

template <typename MapType> std::vector<uint32_t> lookupVector(const MapType& map, uint32_t key) {
    const auto iterator = map.find(key);
    if(iterator == map.end()) {
        return {};
    }

    return iterator->second;
}

} // namespace

void TopologyIndex::buildForShape(uint32_t shapeId, const Geometry::ShapeEntry& entry) {
    ShapeRelations relations;

    for(TopExp_Explorer wire_explorer(entry.shape, TopAbs_WIRE); wire_explorer.More();
        wire_explorer.Next()) {
        const TopoDS_Wire wire = TopoDS::Wire(wire_explorer.Current());
        const int wire_index = entry.wireMap.FindIndex(wire);
        if(wire_index == 0) {
            continue;
        }

        const auto wire_local_id = static_cast<uint32_t>(wire_index);
        auto& wire_edges = relations.wireEdgesMap[wire_local_id];

        for(TopExp_Explorer edge_explorer(wire, TopAbs_EDGE); edge_explorer.More();
            edge_explorer.Next()) {
            const TopoDS_Edge edge = TopoDS::Edge(edge_explorer.Current());
            const int edge_index = entry.edgeMap.FindIndex(edge);
            if(edge_index == 0) {
                continue;
            }

            const auto edge_local_id = static_cast<uint32_t>(edge_index);
            relations.edgeToWireMap[edge_local_id] = wire_local_id;
            wire_edges.push_back(edge_local_id);
        }
    }

    for(TopExp_Explorer face_explorer(entry.shape, TopAbs_FACE); face_explorer.More();
        face_explorer.Next()) {
        const TopoDS_Face face = TopoDS::Face(face_explorer.Current());
        const int face_index = entry.faceMap.FindIndex(face);
        if(face_index == 0) {
            continue;
        }

        const auto face_local_id = static_cast<uint32_t>(face_index);

        for(TopExp_Explorer wire_explorer(face, TopAbs_WIRE); wire_explorer.More();
            wire_explorer.Next()) {
            const TopoDS_Wire wire = TopoDS::Wire(wire_explorer.Current());
            const int wire_index = entry.wireMap.FindIndex(wire);
            if(wire_index == 0) {
                continue;
            }

            relations.wireToFaceMap[static_cast<uint32_t>(wire_index)] = face_local_id;
        }
    }

    for(TopExp_Explorer solid_explorer(entry.shape, TopAbs_SOLID); solid_explorer.More();
        solid_explorer.Next()) {
        const TopoDS_Solid solid = TopoDS::Solid(solid_explorer.Current());
        const int solid_index = entry.solidMap.FindIndex(solid);
        if(solid_index == 0) {
            continue;
        }

        const auto solid_local_id = static_cast<uint32_t>(solid_index);
        auto& solid_faces = relations.solidFacesMap[solid_local_id];

        for(TopExp_Explorer face_explorer(solid, TopAbs_FACE); face_explorer.More();
            face_explorer.Next()) {
            const TopoDS_Face face = TopoDS::Face(face_explorer.Current());
            const int face_index = entry.faceMap.FindIndex(face);
            if(face_index == 0) {
                continue;
            }

            const auto face_local_id = static_cast<uint32_t>(face_index);
            relations.faceToSolidMap[face_local_id] = solid_local_id;
            solid_faces.push_back(face_local_id);
        }
    }

    m_relations[shapeId] = std::move(relations);
}

void TopologyIndex::removeShape(uint32_t shapeId) { m_relations.erase(shapeId); }

std::optional<uint32_t> TopologyIndex::edgeToWire(uint32_t shapeId, uint32_t edgeLocalId) const {
    const auto shape_iterator = m_relations.find(shapeId);
    if(shape_iterator == m_relations.end()) {
        return std::nullopt;
    }

    return lookupValue(shape_iterator->second.edgeToWireMap, edgeLocalId);
}

std::optional<uint32_t> TopologyIndex::wireToFace(uint32_t shapeId, uint32_t wireLocalId) const {
    const auto shape_iterator = m_relations.find(shapeId);
    if(shape_iterator == m_relations.end()) {
        return std::nullopt;
    }

    return lookupValue(shape_iterator->second.wireToFaceMap, wireLocalId);
}

std::optional<uint32_t> TopologyIndex::faceToSolid(uint32_t shapeId, uint32_t faceLocalId) const {
    const auto shape_iterator = m_relations.find(shapeId);
    if(shape_iterator == m_relations.end()) {
        return std::nullopt;
    }

    return lookupValue(shape_iterator->second.faceToSolidMap, faceLocalId);
}

std::vector<uint32_t> TopologyIndex::wireEdges(uint32_t shapeId, uint32_t wireLocalId) const {
    const auto shape_iterator = m_relations.find(shapeId);
    if(shape_iterator == m_relations.end()) {
        return {};
    }

    return lookupVector(shape_iterator->second.wireEdgesMap, wireLocalId);
}

std::vector<uint32_t> TopologyIndex::solidFaces(uint32_t shapeId, uint32_t solidLocalId) const {
    const auto shape_iterator = m_relations.find(shapeId);
    if(shape_iterator == m_relations.end()) {
        return {};
    }

    return lookupVector(shape_iterator->second.solidFacesMap, solidLocalId);
}

} // namespace OpenGeoLab::Scene
