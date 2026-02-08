#include "mesh/mesh_types.hpp"
#include <stdexcept>
#include <unordered_map>

namespace OpenGeoLab::Mesh {
std::string meshElementTypeToString(MeshElementType type) {
    switch(type) {
    case MeshElementType::Invalid:
        return "Invalid";
    case MeshElementType::Node:
        return "Node";
    case MeshElementType::Line:
        return "Line";
    case MeshElementType::Triangle:
        return "Triangle";
    case MeshElementType::Quad4:
        return "Quad4";
    case MeshElementType::Tetra4:
        return "Tetra4";
    case MeshElementType::Hexa8:
        return "Hexa8";
    case MeshElementType::Prism6:
        return "Prism6";
    default:
        break;
    }

    throw std::invalid_argument("Unknown MeshElementType value");
}

MeshElementType meshElementTypeFromString(std::string_view str) {
    static std::unordered_map<std::string_view, MeshElementType> string_to_type = {
        {"Invalid", MeshElementType::Invalid},   {"Line", MeshElementType::Line},
        {"Triangle", MeshElementType::Triangle}, {"Quad4", MeshElementType::Quad4},
        {"Tetra4", MeshElementType::Tetra4},     {"Hexa8", MeshElementType::Hexa8},
        {"Prism6", MeshElementType::Prism6},
    };
    auto it = string_to_type.find(str);
    if(it != string_to_type.end()) {
        return it->second;
    }
    throw std::invalid_argument("Unknown MeshElementType string: " + std::string(str));
}
} // namespace OpenGeoLab::Mesh