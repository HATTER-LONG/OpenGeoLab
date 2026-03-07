/**
 * @file generate_mesh_action.cpp
 * @brief Implementation of GenerateMeshAction
 */

#include "generate_mesh_action.hpp"

#include "../mesh_documentImpl.hpp"
#include "geometry/geometry_document.hpp"
#include "geometry/geometry_entity.hpp"
#include "geometry/geometry_types.hpp"
#include "util/logger.hpp"
#include "util/process_path_guard.hpp"
#include "util/progress_callback.hpp"

#include <BRep_Builder.hxx>
#include <gmsh.h>
#include <kangaroo/util/component_factory.hpp>

#include <map>

namespace OpenGeoLab::Mesh {

#define ERROR_AND_RETURN(msg)                                                                      \
    do {                                                                                           \
        response["success"] = false;                                                               \
        response["error"] = msg;                                                                   \
        return response;                                                                           \
    } while(0)

// =============================================================================
// Anonymous namespace — utility functions and Gmsh pipeline steps
// =============================================================================

namespace {

struct MeshRequestData {
    Geometry::EntityRefSet m_entities;
    GmshMeshContext m_ctx;
};

struct ElementWriteContext {
    std::vector<MeshElement>& m_elements;
    const std::unordered_map<size_t, MeshNodeId>& m_gmshToLocal;
};

struct ElementBatchSpec {
    MeshElementType m_type{MeshElementType::None};
    const std::vector<size_t>* m_nodeIds{nullptr};
    size_t m_stride{0};
    size_t m_elementCount{0};
    uint64_t m_partUid{0};
};

bool reportCancelled(Util::ProgressCallback progress_callback,
                     double progress,
                     const char* message,
                     nlohmann::json& response) {
    if(!progress_callback || progress_callback(progress, message)) {
        return true;
    }
    response["success"] = false;
    response["error"] = "Operation cancelled";
    return false;
}

bool parseEntitiesFromParams(const nlohmann::json& params,
                             Geometry::EntityRefSet& entities,
                             std::string& error) {
    const auto entities_it = params.find("entities");
    if(entities_it == params.end() || !entities_it->is_array()) {
        error = "Missing or invalid 'entities' array";
        return false;
    }

    for(const auto& entity_obj : *entities_it) {
        if(entity_obj.contains("uid") && entity_obj["uid"].is_number_unsigned() &&
           entity_obj.contains("type") && entity_obj["type"].is_string()) {
            const Geometry::EntityUID entity_uid = entity_obj["uid"].get<Geometry::EntityUID>();
            const auto entity_type_opt =
                Geometry::entityTypeFromString(entity_obj["type"].get<std::string>());
            if(!entity_type_opt.has_value()) {
                error = "Invalid entity type in 'entities'";
                return false;
            }

            const Geometry::EntityType entity_type = entity_type_opt.value();
            if(entity_type == Geometry::EntityType::None) {
                error = "Invalid entity type in 'entities'";
                return false;
            }

            entities.emplace(Geometry::EntityRef(entity_uid, entity_type));
        }
    }

    if(entities.empty()) {
        error = "No valid entities provided for meshing";
        return false;
    }

    return true;
}

bool parseMeshSettingsFromParams(const nlohmann::json& params,
                                 GmshMeshContext& ctx,
                                 std::string& error) {
    ctx.m_elementSize = params.value("elementSize", 1.0);
    if(!(ctx.m_elementSize > 0.0)) {
        error = "Invalid 'elementSize' parameter: must be positive number";
        return false;
    }

    ctx.m_elementSizeMin = params.value("elementSizeMin", ctx.m_elementSize);
    ctx.m_elementSizeMax = params.value("elementSizeMax", ctx.m_elementSize * 2.0);
    if(!(ctx.m_elementSizeMin > 0.0) || !(ctx.m_elementSizeMax > 0.0) ||
       ctx.m_elementSizeMax < ctx.m_elementSizeMin) {
        error = "Invalid mesh size range: require 0 < elementSizeMin <= elementSizeMax";
        return false;
    }

    ctx.m_meshDimension = params.value("meshDimension", 2);
    if(ctx.m_meshDimension != 2 && ctx.m_meshDimension != 3) {
        error = "Invalid 'meshDimension': must be 2 or 3";
        return false;
    }

    ctx.m_elementType = params.value("elementType", "triangle");
    if(ctx.m_elementType != "triangle" && ctx.m_elementType != "quad" &&
       ctx.m_elementType != "auto") {
        error = "Invalid 'elementType': must be 'triangle', 'quad', or 'auto'";
        return false;
    }

    ctx.m_algorithm2D = params.value("algorithm2D", "frontal");
    if(ctx.m_algorithm2D != "automatic" && ctx.m_algorithm2D != "meshadapt" &&
       ctx.m_algorithm2D != "delaunay" && ctx.m_algorithm2D != "frontal" &&
       ctx.m_algorithm2D != "bamg" && ctx.m_algorithm2D != "frontal_quad") {
        error = "Invalid 'algorithm2D' parameter";
        return false;
    }

    ctx.m_algorithm3D = params.value("algorithm3D", "delaunay");
    if(ctx.m_algorithm3D != "delaunay" && ctx.m_algorithm3D != "frontal" &&
       ctx.m_algorithm3D != "mmg3d" && ctx.m_algorithm3D != "rtree" && ctx.m_algorithm3D != "hxt") {
        error = "Invalid 'algorithm3D' parameter";
        return false;
    }

    ctx.m_elementOrder = params.value("elementOrder", 1);
    if(ctx.m_elementOrder != 1 && ctx.m_elementOrder != 2) {
        error = "Invalid 'elementOrder': must be 1 or 2";
        return false;
    }

    ctx.m_optimizeMesh = params.value("optimizeMesh", true);

    return true;
}

bool parseRequestData(const nlohmann::json& params, MeshRequestData& request, std::string& error) {
    if(!params.is_object()) {
        error = "Invalid params: expected JSON object";
        return false;
    }

    if(!parseEntitiesFromParams(params, request.m_entities, error)) {
        return false;
    }

    return parseMeshSettingsFromParams(params, request.m_ctx, error);
}

void appendShapeToCompound(const Geometry::GeometryEntityPtr& face_entity,
                           uint64_t part_uid,
                           BRep_Builder& builder,
                           GmshMeshContext& ctx) {
    auto shape = face_entity->shape();
    if(shape.IsNull()) {
        return;
    }

    builder.Add(ctx.m_compound, shape);
    ctx.m_facePartUids.push_back(part_uid);
}

void collectFacesFromPartEntity(const Geometry::GeometryDocumentPtr& doc,
                                const Geometry::EntityRef& entity_ref,
                                BRep_Builder& builder,
                                GmshMeshContext& ctx) {
    auto part_entity = doc->findByUIDAndType(entity_ref.m_uid, entity_ref.m_type);
    if(!part_entity) {
        LOG_WARN("GenerateMeshAction: entity not found: uid={}", entity_ref.m_uid);
        return;
    }

    auto related_faces =
        doc->findRelatedEntities(entity_ref.m_uid, entity_ref.m_type, Geometry::EntityType::Face);
    for(const auto& face_key : related_faces) {
        auto face_entity = doc->findByUIDAndType(face_key.m_uid, Geometry::EntityType::Face);
        if(face_entity) {
            appendShapeToCompound(face_entity, entity_ref.m_uid, builder, ctx);
        }
    }
}

void collectSingleFaceEntity(const Geometry::GeometryDocumentPtr& doc,
                             const Geometry::EntityRef& entity_ref,
                             BRep_Builder& builder,
                             GmshMeshContext& ctx) {
    auto face_entity = doc->findByUIDAndType(entity_ref.m_uid, entity_ref.m_type);
    if(!face_entity) {
        LOG_WARN("GenerateMeshAction: Face not found: uid={}", entity_ref.m_uid);
        return;
    }

    uint64_t part_uid = 0;
    auto parent_parts = doc->findRelatedEntities(entity_ref.m_uid, Geometry::EntityType::Face,
                                                 Geometry::EntityType::Part);
    if(!parent_parts.empty()) {
        part_uid = parent_parts.front().m_uid;
    }

    appendShapeToCompound(face_entity, part_uid, builder, ctx);
}

template <typename Fn> void withGmshSession(Fn&& fn) {
    const WindowsProcessPathGuard path_guard;

    gmsh::initialize();
    gmsh::option::setNumber("General.Terminal", 0);
    gmsh::model::add("mesh_model");

    try {
        fn();
    } catch(...) {
        gmsh::finalize();
        throw;
    }

    gmsh::finalize();
}

/// Map a Gmsh element type code to our MeshElementType.
MeshElementType gmshTypeToMeshElementType(int gmsh_type) {
    switch(gmsh_type) {
    case 1:
        return MeshElementType::Line;
    case 2:
        return MeshElementType::Triangle;
    case 3:
        return MeshElementType::Quad4;
    case 4:
        return MeshElementType::Tetra4;
    case 5:
        return MeshElementType::Hexa8;
    case 6:
        return MeshElementType::Prism6;
    case 7:
        return MeshElementType::Pyramid5;
    default:
        return MeshElementType::None;
    }
}

/// Encode a Gmsh (dim, tag) pair into a single int64_t key.
/// Gmsh entity tags are unique only within the same dimension.
int64_t dimTagKey(int dim, int tag) {
    return (static_cast<int64_t>(dim) << 32) | static_cast<uint32_t>(tag);
}

/// Configure Gmsh meshing algorithm based on element type.
int gmshAlgorithm2DCode(const std::string& algorithm) {
    if(algorithm == "meshadapt") {
        return 1;
    }
    if(algorithm == "automatic") {
        return 2;
    }
    if(algorithm == "delaunay") {
        return 5;
    }
    if(algorithm == "frontal") {
        return 6;
    }
    if(algorithm == "bamg") {
        return 7;
    }
    if(algorithm == "frontal_quad") {
        return 8;
    }
    return 6;
}

int gmshAlgorithm3DCode(const std::string& algorithm) {
    if(algorithm == "frontal") {
        return 4;
    }
    if(algorithm == "mmg3d") {
        return 7;
    }
    if(algorithm == "rtree") {
        return 9;
    }
    if(algorithm == "hxt") {
        return 10;
    }
    return 1;
}

void configureGmshAlgorithm(const GmshMeshContext& ctx) {
    gmsh::option::setNumber("Mesh.MeshSizeMin", ctx.m_elementSizeMin);
    gmsh::option::setNumber("Mesh.MeshSizeMax", ctx.m_elementSizeMax);
    gmsh::option::setNumber("Mesh.ElementOrder", ctx.m_elementOrder);
    gmsh::option::setNumber("Mesh.RecombineAll", ctx.m_elementType == "quad" ? 1 : 0);

    if(ctx.m_meshDimension == 3) {
        gmsh::option::setNumber("Mesh.Algorithm3D", gmshAlgorithm3DCode(ctx.m_algorithm3D));
    } else {
        const bool wants_quad = ctx.m_elementType == "quad";
        const std::string algorithm =
            wants_quad && ctx.m_algorithm2D == "frontal" ? "frontal_quad" : ctx.m_algorithm2D;
        gmsh::option::setNumber("Mesh.Algorithm", gmshAlgorithm2DCode(algorithm));
    }

    if(ctx.m_elementType == "quad") {
        gmsh::option::setNumber("Mesh.RecombinationAlgorithm", 1);
    }
}

nlohmann::json buildMeshSummary(const GmshMeshContext& ctx) {
    std::map<std::string, size_t> counts_by_type;
    for(const auto& element : ctx.m_elements) {
        if(auto type = meshElementTypeToString(element.elementType()); type.has_value()) {
            counts_by_type[type.value()] += 1;
        }
    }

    nlohmann::json mesh_entities = nlohmann::json::array();
    for(const auto& [type, count] : counts_by_type) {
        mesh_entities.push_back({{"type", type}, {"count", count}});
    }

    return {{"generatedNodeCount", ctx.m_nodes.size()},
            {"generatedElementCount", ctx.m_elements.size()},
            {"mesh_entities", std::move(mesh_entities)}};
}

/// Import the OCC compound into Gmsh and build (dim,tag) -> Part UID mapping.
void importAndMapShape(GmshMeshContext& ctx) {
    std::vector<std::pair<int, int>> out_dim_tags;
    gmsh::model::occ::importShapesNativePointer(static_cast<const void*>(&ctx.m_compound),
                                                out_dim_tags);
    gmsh::model::occ::synchronize();

    LOG_DEBUG("GenerateMeshAction: Imported {} dim-tags into Gmsh", out_dim_tags.size());

    // The dim-2 entries in out_dim_tags correspond 1:1 (by order) to the faces
    // added to the compound, giving us the Gmsh tag -> Part UID mapping.
    size_t face_idx = 0;
    for(const auto& [dim, tag] : out_dim_tags) {
        if(dim == 2 && face_idx < ctx.m_facePartUids.size()) {
            ctx.m_tagToPartUid[dimTagKey(dim, tag)] = ctx.m_facePartUids[face_idx];
            ++face_idx;
        }
    }
    LOG_DEBUG("GenerateMeshAction: Mapped {} Gmsh face entities to Part UIDs",
              ctx.m_tagToPartUid.size());
}

/// Extract nodes from Gmsh and add them to the mesh document.
void extractNodes(GmshMeshContext& ctx) {
    std::vector<std::size_t> node_tags;
    std::vector<double> node_coords;
    std::vector<double> parametric_coords;
    gmsh::model::mesh::getNodes(node_tags, node_coords, parametric_coords);

    ctx.m_nodes.reserve(node_tags.size());
    ctx.m_gmshToLocal.reserve(node_tags.size());

    for(size_t i = 0; i < node_tags.size(); ++i) {
        MeshNode node(node_coords[3 * i], node_coords[3 * i + 1], node_coords[3 * i + 2]);
        ctx.m_gmshToLocal[node_tags[i]] = node.nodeId();
        ctx.m_nodes.emplace_back(std::move(node));
    }

    LOG_DEBUG("GenerateMeshAction: Extracted {} nodes", node_tags.size());
}

/// Add a batch of elements of one Gmsh type to the mesh document.
void addElementsOfType(const ElementWriteContext& write_ctx, const ElementBatchSpec& batch) {
    if(batch.m_nodeIds == nullptr) {
        return;
    }

    for(size_t ei = 0; ei < batch.m_elementCount; ++ei) {
        MeshElement elem(batch.m_type);
        elem.setPartUid(batch.m_partUid);
        const uint8_t nc = elem.nodeCount();
        for(uint8_t ni = 0; ni < nc; ++ni) {
            const size_t idx = ei * batch.m_stride + ni;
            if(idx >= batch.m_nodeIds->size()) {
                break;
            }
            if(auto it = write_ctx.m_gmshToLocal.find((*batch.m_nodeIds)[idx]);
               it != write_ctx.m_gmshToLocal.end()) {
                elem.setNodeId(ni, it->second);
            }
        }
        write_ctx.m_elements.emplace_back(std::move(elem));
    }
}

/// Extract elements from Gmsh per entity and add them to the mesh document.
void extractElements(GmshMeshContext& ctx) {
    std::vector<std::pair<int, int>> all_entities;
    gmsh::model::getEntities(all_entities);
    const ElementWriteContext write_ctx{ctx.m_elements, ctx.m_gmshToLocal};

    for(const auto& [dim, tag] : all_entities) {
        std::vector<int> element_types;
        std::vector<std::vector<size_t>> element_tags;
        std::vector<std::vector<size_t>> element_node_tags;
        gmsh::model::mesh::getElements(element_types, element_tags, element_node_tags, dim, tag);

        size_t entity_element_count = 0;
        for(const auto& tags : element_tags) {
            entity_element_count += tags.size();
        }
        ctx.m_elements.reserve(ctx.m_elements.size() + entity_element_count);

        // Look up Part UID for this Gmsh entity
        uint64_t part_uid = 0;
        if(auto it = ctx.m_tagToPartUid.find(dimTagKey(dim, tag)); it != ctx.m_tagToPartUid.end()) {
            part_uid = it->second;
        }

        for(size_t ti = 0; ti < element_types.size(); ++ti) {
            const MeshElementType our_type = gmshTypeToMeshElementType(element_types[ti]);
            if(our_type == MeshElementType::None) {
                LOG_DEBUG("GenerateMeshAction: Skipping unsupported Gmsh element type {}",
                          element_types[ti]);
                continue;
            }

            // Query Gmsh for the number of nodes per element of this type
            std::string elem_name;
            int elem_dim = 0, order = 0, num_nodes = 0, num_primary = 0;
            std::vector<double> param_coords;
            gmsh::model::mesh::getElementProperties(element_types[ti], elem_name, elem_dim, order,
                                                    num_nodes, param_coords, num_primary);

            const ElementBatchSpec batch{our_type, &element_node_tags[ti],
                                         static_cast<size_t>(num_nodes), element_tags[ti].size(),
                                         part_uid};
            addElementsOfType(write_ctx, batch);
        }
    }

    LOG_DEBUG("GenerateMeshAction: Extracted elements, total: {}", ctx.m_elements.size());
}

} // anonymous namespace

// =============================================================================
// GenerateMeshAction — public interface
// =============================================================================

nlohmann::json GenerateMeshAction::execute(const nlohmann::json& params,
                                           Util::ProgressCallback progress_callback) {
    nlohmann::json response;

    LOG_TRACE("GenerateMeshAction: Executing with params: {}", params.dump());
    if(!reportCancelled(progress_callback, 0.05, "Starting parse parameters...", response)) {
        return response;
    }

    MeshRequestData request;
    std::string error;
    if(!parseRequestData(params, request, error)) {
        ERROR_AND_RETURN(error);
    }

    // --- Build compound shape from selected entities ---
    collectFaceShapes(request.m_entities, request.m_ctx);

    if(!reportCancelled(progress_callback, 0.2, "Starting mesh generation...", response)) {
        return response;
    }

    // --- Run Gmsh pipeline ---
    try {
        bool pipeline_completed = false;
        withGmshSession(
            [&]() { pipeline_completed = runGmshPipeline(request.m_ctx, progress_callback); });
        if(!pipeline_completed) {
            response["success"] = false;
            response["error"] = "Operation cancelled";
            return response;
        }
    } catch(const std::exception& e) {
        LOG_ERROR("GenerateMeshAction: Gmsh error: {}", e.what());
        ERROR_AND_RETURN(std::string("Gmsh import failed: ") + e.what());
    }

    auto mesh_doc = MeshDocumentImpl::instance();
    if(!mesh_doc) {
        ERROR_AND_RETURN("No active mesh document");
    }

    const bool replace_existing = params.value("replaceExisting", false);
    const auto summary = buildMeshSummary(request.m_ctx);
    const bool committed =
        replace_existing ? mesh_doc->replaceMeshData(std::move(request.m_ctx.m_nodes),
                                                     std::move(request.m_ctx.m_elements), error)
                         : mesh_doc->appendMeshData(std::move(request.m_ctx.m_nodes),
                                                    std::move(request.m_ctx.m_elements), error);
    if(!committed) {
        LOG_ERROR("GenerateMeshAction: Failed to commit mesh data: {}", error);
        ERROR_AND_RETURN(error);
    }

    LOG_INFO("GenerateMeshAction: Generated {} nodes, {} elements", mesh_doc->nodeCount(),
             mesh_doc->elementCount());

    response["success"] = true;
    response["replaceExisting"] = replace_existing;
    response["nodeCount"] = mesh_doc->nodeCount();
    response["elementCount"] = mesh_doc->elementCount();
    response["generatedNodeCount"] = summary["generatedNodeCount"];
    response["generatedElementCount"] = summary["generatedElementCount"];
    response["mesh_entities"] = summary["mesh_entities"];
    if(progress_callback) {
        progress_callback(1.0, "Mesh generation complete");
    }
    return response;
}

// =============================================================================
// GenerateMeshAction — private methods
// =============================================================================

void GenerateMeshAction::collectFaceShapes(const Geometry::EntityRefSet& entities,
                                           GmshMeshContext& ctx) {
    auto doc = GeoDocumentInstance;

    BRep_Builder builder;
    builder.MakeCompound(ctx.m_compound);

    for(const auto& entity_ref : entities) {
        if(entity_ref.m_type == Geometry::EntityType::Part ||
           entity_ref.m_type == Geometry::EntityType::Solid) {
            collectFacesFromPartEntity(doc, entity_ref, builder, ctx);
        } else if(entity_ref.m_type == Geometry::EntityType::Face) {
            collectSingleFaceEntity(doc, entity_ref, builder, ctx);
        }
    }

    LOG_INFO("GenerateMeshAction: Collected {} face shapes for meshing", ctx.m_facePartUids.size());
}

bool GenerateMeshAction::runGmshPipeline(GmshMeshContext& ctx,
                                         Util::ProgressCallback progress_callback) {
    importAndMapShape(ctx);

    if(progress_callback && !progress_callback(0.3, "Configuring mesh parameters...")) {
        return false;
    }

    configureGmshAlgorithm(ctx);

    if(progress_callback && !progress_callback(0.4, "Running Gmsh mesh generation...")) {
        return false;
    }

    gmsh::model::mesh::generate(ctx.m_meshDimension);

    if(ctx.m_optimizeMesh) {
        gmsh::model::mesh::optimize("Netgen");
    }

    if(progress_callback && !progress_callback(0.6, "Extracting nodes...")) {
        return false;
    }

    extractNodes(ctx);

    if(progress_callback && !progress_callback(0.75, "Extracting elements...")) {
        return false;
    }

    extractElements(ctx);

    if(progress_callback && !progress_callback(0.9, "Mesh extraction complete")) {
        return false;
    }

    return true;
}

} // namespace OpenGeoLab::Mesh
