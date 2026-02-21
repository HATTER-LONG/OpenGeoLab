/**
 * @file pick_entity_type.cpp
 * @brief Implementation of PickEntityType string conversion
 */

#include "render/pick_entity_type.hpp"

#include <stdexcept>
#include <unordered_map>

namespace OpenGeoLab::Render {

std::string pickEntityTypeToString(PickEntityType t) {
    switch(t) {
    case PickEntityType::None:
        return "None";
    case PickEntityType::Vertex:
        return "Vertex";
    case PickEntityType::Edge:
        return "Edge";
    case PickEntityType::Wire:
        return "Wire";
    case PickEntityType::Face:
        return "Face";
    case PickEntityType::Shell:
        return "Shell";
    case PickEntityType::Solid:
        return "Solid";
    case PickEntityType::CompSolid:
        return "CompSolid";
    case PickEntityType::Compound:
        return "Compound";
    case PickEntityType::Part:
        return "Part";
    case PickEntityType::MeshNode:
        return "MeshNode";
    case PickEntityType::MeshElement:
        return "MeshElement";
    default:
        break;
    }
    return "Unknown";
}

PickEntityType pickEntityTypeFromString(std::string_view str) {
    static const std::unordered_map<std::string_view, PickEntityType> s_map = {
        {"None", PickEntityType::None},         {"Vertex", PickEntityType::Vertex},
        {"Edge", PickEntityType::Edge},         {"Wire", PickEntityType::Wire},
        {"Face", PickEntityType::Face},         {"Shell", PickEntityType::Shell},
        {"Solid", PickEntityType::Solid},       {"CompSolid", PickEntityType::CompSolid},
        {"Compound", PickEntityType::Compound}, {"Part", PickEntityType::Part},
        {"MeshNode", PickEntityType::MeshNode}, {"MeshElement", PickEntityType::MeshElement},
    };
    auto it = s_map.find(str);
    if(it != s_map.end()) {
        return it->second;
    }
    throw std::invalid_argument("Unknown PickEntityType string: " + std::string(str));
}

} // namespace OpenGeoLab::Render
