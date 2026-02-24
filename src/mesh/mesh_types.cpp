#include "mesh/mesh_types.hpp"
#include <array>
#include <atomic>
#include <stdexcept>
#include <unordered_map>

namespace OpenGeoLab::Mesh {

// =============================================================================
// MeshElementType conversions
// =============================================================================

std::optional<std::string> meshElementTypeToString(MeshElementType type) noexcept {
    switch(type) {
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
    case MeshElementType::Pyramid5:
        return "Pyramid5";
    default:
        break;
    }

    return std::nullopt;
}

std::optional<MeshElementType> meshElementTypeFromString(std::string_view str) noexcept {
    static std::unordered_map<std::string_view, MeshElementType> string_to_type = {
        {"Node", MeshElementType::Node},         {"Line", MeshElementType::Line},
        {"Triangle", MeshElementType::Triangle}, {"Quad4", MeshElementType::Quad4},
        {"Tetra4", MeshElementType::Tetra4},     {"Hexa8", MeshElementType::Hexa8},
        {"Prism6", MeshElementType::Prism6},     {"Pyramid5", MeshElementType::Pyramid5},
    };
    auto it = string_to_type.find(str);
    if(it != string_to_type.end()) {
        return it->second;
    }
    return std::nullopt;
}
namespace {
std::atomic<MeshElementId> g_mesh_element_id_counter{1};
std::array<std::atomic<MeshElementUID>, 8> g_mesh_element_uid_counters = {
    std::atomic<MeshElementUID>{1}, // Node
    std::atomic<MeshElementUID>{1}, // Line
    std::atomic<MeshElementUID>{1}, // Triangle
    std::atomic<MeshElementUID>{1}, // Quad4
    std::atomic<MeshElementUID>{1}, // Tetra4
    std::atomic<MeshElementUID>{1}, // Hexa8
    std::atomic<MeshElementUID>{1}, // Prism6
    std::atomic<MeshElementUID>{1}  // Pyramid5
};
} // namespace

[[nodiscard]] MeshElementId generateMeshElementId() {
    return g_mesh_element_id_counter.fetch_add(1, std::memory_order_relaxed);
}

[[nodiscard]] MeshElementUID generateMeshElementUID(MeshElementType type) {
    size_t index = static_cast<size_t>(type);
    if(index >= g_mesh_element_uid_counters.size()) {
        throw std::invalid_argument("Invalid MeshElementType for UID generation");
    }
    return g_mesh_element_uid_counters[index].fetch_add(1, std::memory_order_relaxed);
}

void resetMeshElementIdGenerator() {
    g_mesh_element_id_counter.store(1, std::memory_order_relaxed);
}
void resetMeshElementUIDGenerator(MeshElementType type) {
    size_t index = static_cast<size_t>(type);
    if(index >= g_mesh_element_uid_counters.size()) {
        throw std::invalid_argument("Invalid MeshElementType for UID reset");
    }
    g_mesh_element_uid_counters[index].store(1, std::memory_order_relaxed);
}
void resetAllMeshElementUIDGenerators() {
    for(auto& counter : g_mesh_element_uid_counters) {
        counter.store(1, std::memory_order_relaxed);
    }
}
uint64_t getCurrentMeshElementIdCounter() {
    return g_mesh_element_id_counter.load(std::memory_order_relaxed);
}

uint64_t getCurrentMeshElementUIDCounter(MeshElementType type) {
    size_t index = static_cast<size_t>(type);
    if(index >= g_mesh_element_uid_counters.size()) {
        throw std::invalid_argument("Invalid MeshElementType for UID counter retrieval");
    }
    return g_mesh_element_uid_counters[index].load(std::memory_order_relaxed);
}

} // namespace OpenGeoLab::Mesh