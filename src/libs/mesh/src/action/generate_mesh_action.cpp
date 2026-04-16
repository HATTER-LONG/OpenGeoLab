/**
 * @file generate_mesh_action.cpp
 * @brief GenerateMeshAction implementation
 */

#include "generate_mesh_action.hpp"

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/geometry/shape_store.hpp>
#include <opengeolab/mesh/mesh_entry.hpp>
#include <opengeolab/mesh/mesh_store.hpp>

#include <BRepBndLib.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Shape.hxx>
#include <gmsh.h>
extern "C" {
#include <gmshc.h>
}

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace OpenGeoLab::Mesh {

namespace {

struct RequestedEntity {
    uint32_t shapeId{0};
    Core::EntityType type{Core::EntityType::GeoFace};
    uint32_t localId{0};
};

struct MeshSettings {
    double minSize{1.0};
    double maxSize{1.0};
    int dimension{2};
    int order{1};
    bool recombine{false};
    bool optimize{false};
    int algorithmCode{5};
    std::string sizeMode{"absolute"};
};

class GmshSession final {
public:
    GmshSession() {
        int ierr = 0;
        m_owner = gmshIsInitialized(&ierr) == 0;
        m_ok = ierr == 0;
        if(m_owner && m_ok) {
            gmshInitialize(0, nullptr, 1, 0, &ierr);
            m_ok = ierr == 0;
        }
    }

    ~GmshSession() {
        if(!m_owner || !m_ok) {
            return;
        }
        int ierr = 0;
        gmshFinalize(&ierr);
        static_cast<void>(ierr);
    }

    [[nodiscard]] bool ok() const noexcept { return m_ok; }

private:
    bool m_owner{false};
    bool m_ok{false};
};

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](const unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

std::optional<Core::EntityType> parseGeometryEntityType(const std::string& type_name) {
    if(type_name == "GeoFace") {
        return Core::EntityType::GeoFace;
    }
    if(type_name == "GeoSolid") {
        return Core::EntityType::GeoSolid;
    }
    return std::nullopt;
}

std::optional<int> meshAlgorithmCode(const std::string& algorithm_name, const int dimension) {
    const auto lower_name = toLower(algorithm_name);
    if(dimension == 2) {
        if(lower_name == "automatic") {
            return 2;
        }
        if(lower_name == "meshadapt") {
            return 1;
        }
        if(lower_name == "delaunay") {
            return 5;
        }
        if(lower_name == "frontal") {
            return 6;
        }
        if(lower_name == "bamg") {
            return 7;
        }
        if(lower_name == "frontal_quad") {
            return 8;
        }
        return std::nullopt;
    }

    if(lower_name == "delaunay") {
        return 1;
    }
    if(lower_name == "frontal") {
        return 4;
    }
    if(lower_name == "mmg3d") {
        return 7;
    }
    if(lower_name == "rtree") {
        return 9;
    }
    if(lower_name == "hxt") {
        return 10;
    }
    return std::nullopt;
}

std::optional<MeshElementType> mapGmshElementType(const int gmsh_type) {
    switch(gmsh_type) {
    case 2:
        return MeshElementType::Triangle;
    case 3:
        return MeshElementType::Quad;
    case 4:
        return MeshElementType::Tetra;
    case 5:
        return MeshElementType::Hexa;
    case 6:
        return MeshElementType::Prism;
    case 7:
        return MeshElementType::Pyramid;
    case 9:
        return MeshElementType::Tri6;
    case 10:
        return MeshElementType::Quad9;
    case 16:
        return MeshElementType::Quad8;
    default:
        return std::nullopt;
    }
}

std::optional<MeshSettings> parseSettings(const nlohmann::json& param, std::string& error) {
    MeshSettings settings;
    const double element_size = param.value("elementSize", 1.0);
    settings.dimension = param.value("dimension", 2);
    settings.sizeMode = toLower(param.value("sizeMode", std::string{"absolute"}));
    settings.recombine = toLower(param.value("elementType", std::string{"triangle"})) == "quad";

    if(settings.sizeMode != "absolute" && settings.sizeMode != "percentage") {
        error = "sizeMode must be 'absolute' or 'percentage'";
        return std::nullopt;
    }

    if(settings.dimension != 2 && settings.dimension != 3) {
        error = "dimension must be 2 or 3";
        return std::nullopt;
    }

    const auto element_type = toLower(param.value("elementType", std::string{"triangle"}));
    if(element_type != "triangle" && element_type != "quad") {
        error = "elementType must be triangle or quad";
        return std::nullopt;
    }

    const auto advanced = param.contains("advanced") && param["advanced"].is_object()
                              ? param["advanced"]
                              : nlohmann::json::object();
    settings.minSize = advanced.value("minSize", element_size);
    settings.maxSize = advanced.value("maxSize", element_size);
    settings.order = advanced.value("order", 1);
    settings.optimize = advanced.value("optimize", false);
    if(settings.minSize > settings.maxSize) {
        std::swap(settings.minSize, settings.maxSize);
    }

    const auto algorithm_name = param.value("algorithm", std::string{"delaunay"});
    const auto algorithm_code = meshAlgorithmCode(algorithm_name, settings.dimension);
    if(!algorithm_code.has_value()) {
        error = "Unsupported algorithm for requested mesh dimension";
        return std::nullopt;
    }
    settings.algorithmCode = *algorithm_code;
    return settings;
}

std::optional<RequestedEntity> parseRequestedEntity(const nlohmann::json& entity_json,
                                                    std::string& error) {
    const auto type_name = entity_json.value("type", std::string{});
    const auto entity_type = parseGeometryEntityType(type_name);
    if(!entity_type.has_value()) {
        error = "Only GeoFace and GeoSolid entities are supported";
        return std::nullopt;
    }

    const auto shape_id = entity_json.value("shapeId", static_cast<uint32_t>(0));
    const auto local_id = entity_json.value("localId", static_cast<uint32_t>(0));
    if(local_id == 0) {
        error = "Each entity must include a non-zero localId";
        return std::nullopt;
    }

    return RequestedEntity{shape_id, *entity_type, local_id};
}

std::string gmshLastError() {
    char* message = nullptr;
    int ierr = 0;
    gmshLoggerGetLastError(&message, &ierr);
    if(ierr != 0 || message == nullptr) {
        return {};
    }

    const std::string result = message;
    gmshFree(message);
    return result;
}

bool checkGmsh(const int ierr, std::string& error, const std::string_view fallback) {
    if(ierr == 0) {
        return true;
    }

    error = gmshLastError();
    if(error.empty()) {
        error = std::string(fallback);
    }
    return false;
}

void freeSizeT(size_t*& values) {
    if(values != nullptr) {
        gmshFree(values);
        values = nullptr;
    }
}

void freeDouble(double*& values) {
    if(values != nullptr) {
        gmshFree(values);
        values = nullptr;
    }
}

void freeInt(int*& values) {
    if(values != nullptr) {
        gmshFree(values);
        values = nullptr;
    }
}

void freeNestedSizeT(size_t**& values, size_t*& sizes, const size_t count) {
    if(values != nullptr) {
        for(size_t index = 0; index < count; ++index) {
            if(values[index] != nullptr) {
                gmshFree(values[index]);
            }
        }
        gmshFree(values);
        values = nullptr;
    }

    if(sizes != nullptr) {
        gmshFree(sizes);
        sizes = nullptr;
    }
}

bool buildMeshEntry(const TopoDS_Compound& compound,
                    const uint32_t shape_id,
                    const MeshSettings& settings,
                    MeshEntry& entry,
                    std::string& error) {
    GmshSession session;
    if(!session.ok()) {
        error = "Failed to initialize gmsh";
        return false;
    }

    int ierr = 0;
    gmshOptionSetNumber("General.Terminal", 0, &ierr);
    if(!checkGmsh(ierr, error, "Failed to disable gmsh terminal output")) {
        return false;
    }

    gmshClear(&ierr);
    if(!checkGmsh(ierr, error, "gmsh clear failed")) {
        return false;
    }

    gmshModelAdd("mesh", &ierr);
    if(!checkGmsh(ierr, error, "Failed to create gmsh model")) {
        return false;
    }

    int* imported_entities = nullptr;
    size_t imported_entities_n = 0;
    gmshModelOccImportShapesNativePointer(&compound, &imported_entities, &imported_entities_n, 0,
                                          &ierr);
    if(!checkGmsh(ierr, error, "Failed to import OCC shapes into gmsh")) {
        freeInt(imported_entities);
        return false;
    }
    freeInt(imported_entities);

    gmshModelOccSynchronize(&ierr);
    if(!checkGmsh(ierr, error, "Failed to synchronize gmsh OCC model")) {
        return false;
    }

    auto actual_settings = settings;
    if(actual_settings.sizeMode == "percentage") {
        Bnd_Box bbox;
        BRepBndLib::Add(compound, bbox);
        if(!bbox.IsVoid()) {
            const double diagonal = std::sqrt(bbox.SquareExtent());
            actual_settings.minSize = diagonal * (actual_settings.minSize / 100.0);
            actual_settings.maxSize = diagonal * (actual_settings.maxSize / 100.0);
        }
    }

    gmshOptionSetNumber("Mesh.MeshSizeMin", actual_settings.minSize, &ierr);
    if(!checkGmsh(ierr, error, "Failed to set Mesh.MeshSizeMin")) {
        return false;
    }
    gmshOptionSetNumber("Mesh.MeshSizeMax", actual_settings.maxSize, &ierr);
    if(!checkGmsh(ierr, error, "Failed to set Mesh.MeshSizeMax")) {
        return false;
    }
    gmshOptionSetNumber("Mesh.ElementOrder", actual_settings.order, &ierr);
    if(!checkGmsh(ierr, error, "Failed to set Mesh.ElementOrder")) {
        return false;
    }
    gmshOptionSetNumber("Mesh.SecondOrderIncomplete", actual_settings.order >= 2 ? 1 : 0, &ierr);
    if(!checkGmsh(ierr, error, "Failed to set Mesh.SecondOrderIncomplete")) {
        return false;
    }
    gmshOptionSetNumber("Mesh.RecombineAll", actual_settings.recombine ? 1 : 0, &ierr);
    if(!checkGmsh(ierr, error, "Failed to set Mesh.RecombineAll")) {
        return false;
    }

    if(actual_settings.dimension == 2) {
        gmshOptionSetNumber("Mesh.Algorithm", actual_settings.algorithmCode, &ierr);
        if(!checkGmsh(ierr, error, "Failed to set Mesh.Algorithm")) {
            return false;
        }
    } else {
        gmshOptionSetNumber("Mesh.Algorithm3D", actual_settings.algorithmCode, &ierr);
        if(!checkGmsh(ierr, error, "Failed to set Mesh.Algorithm3D")) {
            return false;
        }
    }

    gmshModelMeshGenerate(actual_settings.dimension, &ierr);
    if(!checkGmsh(ierr, error, "gmsh mesh generation failed")) {
        return false;
    }

    if(actual_settings.optimize) {
        gmshModelMeshOptimize("Netgen", 1, -1, nullptr, 0, &ierr);
        if(!checkGmsh(ierr, error, "gmsh mesh optimization failed")) {
            return false;
        }
    }

    size_t* node_tags = nullptr;
    size_t node_tags_n = 0;
    double* coordinates = nullptr;
    size_t coordinates_n = 0;
    double* parametric_coordinates = nullptr;
    size_t parametric_coordinates_n = 0;
    gmshModelMeshGetNodes(&node_tags, &node_tags_n, &coordinates, &coordinates_n,
                          &parametric_coordinates, &parametric_coordinates_n, -1, -1, 0, 0, &ierr);
    if(!checkGmsh(ierr, error, "Failed to query gmsh nodes")) {
        freeSizeT(node_tags);
        freeDouble(coordinates);
        freeDouble(parametric_coordinates);
        return false;
    }

    entry = {};
    entry.shapeId = shape_id;
    entry.nodes.reserve(node_tags_n);

    std::unordered_map<std::size_t, uint32_t> node_local_ids;
    node_local_ids.reserve(node_tags_n);
    for(std::size_t index = 0; index < node_tags_n; ++index) {
        MeshNode node{};
        node.position[0] = static_cast<float>(coordinates[index * 3]);
        node.position[1] = static_cast<float>(coordinates[index * 3 + 1]);
        node.position[2] = static_cast<float>(coordinates[index * 3 + 2]);
        entry.nodes.push_back(node);
        node_local_ids.emplace(node_tags[index], static_cast<uint32_t>(index + 1));
    }
    freeSizeT(node_tags);
    freeDouble(coordinates);
    freeDouble(parametric_coordinates);

    int* all_entities = nullptr;
    size_t all_entities_n = 0;
    gmshModelGetEntities(&all_entities, &all_entities_n, -1, &ierr);
    if(!checkGmsh(ierr, error, "Failed to query gmsh entities")) {
        freeInt(all_entities);
        return false;
    }

    for(std::size_t entity_index = 0; entity_index + 1 < all_entities_n; entity_index += 2) {
        const int entity_dim = all_entities[entity_index];
        const int entity_tag = all_entities[entity_index + 1];

        int* element_types = nullptr;
        size_t element_types_n = 0;
        size_t** element_tags = nullptr;
        size_t* element_tags_n = nullptr;
        size_t element_tags_nn = 0;
        size_t** element_node_tags = nullptr;
        size_t* element_node_tags_n = nullptr;
        size_t element_node_tags_nn = 0;
        gmshModelMeshGetElements(&element_types, &element_types_n, &element_tags, &element_tags_n,
                                 &element_tags_nn, &element_node_tags, &element_node_tags_n,
                                 &element_node_tags_nn, entity_dim, entity_tag, &ierr);
        if(!checkGmsh(ierr, error, "Failed to query gmsh elements")) {
            freeInt(all_entities);
            freeInt(element_types);
            freeNestedSizeT(element_tags, element_tags_n, element_tags_nn);
            freeNestedSizeT(element_node_tags, element_node_tags_n, element_node_tags_nn);
            return false;
        }

        for(std::size_t type_index = 0; type_index < element_types_n; ++type_index) {
            const auto mesh_type = mapGmshElementType(element_types[type_index]);
            if(!mesh_type.has_value()) {
                continue;
            }

            const auto nodes_per_element = nodeCount(*mesh_type);
            const auto* connectivity = element_node_tags[type_index];
            const auto connectivity_n = element_node_tags_n[type_index];
            if(nodes_per_element == 0 || connectivity == nullptr || connectivity_n == 0) {
                continue;
            }

            const auto element_count = connectivity_n / nodes_per_element;
            for(std::size_t element_index = 0; element_index < element_count; ++element_index) {
                MeshElement element{};
                element.type = *mesh_type;

                bool valid_element = true;
                for(std::size_t node_index = 0; node_index < nodes_per_element; ++node_index) {
                    const auto gmsh_node_tag =
                        connectivity[element_index * nodes_per_element + node_index];
                    const auto local_id_it = node_local_ids.find(gmsh_node_tag);
                    if(local_id_it == node_local_ids.end()) {
                        valid_element = false;
                        break;
                    }
                    element.nodeLocalIds[node_index] = local_id_it->second;
                }

                if(valid_element) {
                    entry.elements.push_back(element);
                }
            }
        }

        freeInt(element_types);
        freeNestedSizeT(element_tags, element_tags_n, element_tags_nn);
        freeNestedSizeT(element_node_tags, element_node_tags_n, element_node_tags_nn);
    }

    freeInt(all_entities);
    return true;
}

} // namespace

GenerateMeshAction::GenerateMeshAction(MeshStore& mesh_store, Geometry::ShapeStore& shape_store)
    : m_meshStore(mesh_store), m_shapeStore(shape_store) {}

GenerateMeshAction::~GenerateMeshAction() = default;

nlohmann::json GenerateMeshAction::describe() const {
    return {{"name", ACTION_NAME},
            {"description", "Generate mesh from geometry faces/solids using gmsh."},
            {"params",
             {{"entities",
               {{"type", "array"},
                {"required", true},
                {"description", "Array of {shapeId, type, localId} — GeoFace or GeoSolid."}}},
              {"elementSize",
               {{"type", "number"},
                {"required", false},
                {"description", "Target element size (default 1.0)."}}},
              {"dimension",
               {{"type", "integer"},
                {"required", false},
                {"description", "Mesh dimension 2 or 3 (default 2)."}}},
              {"elementType",
               {{"type", "string"},
                {"required", false},
                {"description", "triangle or quad (default triangle)."}}},
              {"algorithm",
               {{"type", "string"},
                {"required", false},
                {"description", "Meshing algorithm name (default delaunay)."}}},
              {"sizeMode",
               {{"type", "string"},
                {"required", false},
                {"description",
                 "Size mode: 'absolute' (default) or 'percentage' of bounding box diagonal."}}},
              {"advanced",
               {{"type", "object"},
                {"required", false},
                {"description", "Advanced params: minSize, maxSize, order, optimize."}}}}},
            {"returns",
             {{"ok", {{"type", "boolean"}, {"description", "true on success."}}},
              {"action", {{"type", "string"}, {"description", "Echo of action name."}}},
              {"results",
               {{"type", "array"},
                {"description", "Per-shape {shapeId, nodeCount, elementCount}."}}}}}};
}

nlohmann::json GenerateMeshAction::execute(const nlohmann::json& param,
                                           const Core::ProgressCallback& progress) {
    const auto entities_json = param.value("entities", nlohmann::json::array());
    if(!entities_json.is_array() || entities_json.empty()) {
        return {{"ok", false},
                {"action", ACTION_NAME},
                {"summary", "entities array is required and must not be empty"}};
    }

    std::string error;
    const auto settings = parseSettings(param, error);
    if(!settings.has_value()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"summary", error}};
    }

    std::unordered_map<uint32_t, std::vector<RequestedEntity>> grouped_entities;
    for(const auto& entity_json : entities_json) {
        const auto entity = parseRequestedEntity(entity_json, error);
        if(!entity.has_value()) {
            return {{"ok", false}, {"action", ACTION_NAME}, {"summary", error}};
        }
        grouped_entities[entity->shapeId].push_back(*entity);
    }

    nlohmann::json results = nlohmann::json::array();
    std::size_t processed_groups = 0;
    for(const auto& [shape_id, group] : grouped_entities) {
        if(progress) {
            progress(static_cast<double>(processed_groups) /
                         static_cast<double>(grouped_entities.size()),
                     "Generating mesh...");
        }

        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);

        for(const auto& entity : group) {
            const auto sub_shape = m_shapeStore.subShape(shape_id, entity.type, entity.localId);
            if(sub_shape.IsNull()) {
                return {{"ok", false},
                        {"action", ACTION_NAME},
                        {"summary", "Failed to resolve requested geometry entity"}};
            }
            builder.Add(compound, sub_shape);
        }

        MeshEntry entry;
        if(!buildMeshEntry(compound, shape_id, *settings, entry, error)) {
            return {{"ok", false}, {"action", ACTION_NAME}, {"summary", error}};
        }

        m_meshStore.setMesh(shape_id, std::move(entry));
        const auto* stored_entry = m_meshStore.find(shape_id);
        const auto node_count = stored_entry != nullptr ? stored_entry->nodes.size() : 0;
        const auto element_count = stored_entry != nullptr ? stored_entry->elements.size() : 0;
        results.push_back(
            {{"shapeId", shape_id}, {"nodeCount", node_count}, {"elementCount", element_count}});

        ++processed_groups;
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", ACTION_NAME}, {"results", std::move(results)}};
}

} // namespace OpenGeoLab::Mesh
