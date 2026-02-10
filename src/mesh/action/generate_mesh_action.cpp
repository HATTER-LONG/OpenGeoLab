/**
 * @file generate_mesh_action.cpp
 * @brief Implementation of GenerateMeshAction
 */

#include "generate_mesh_action.hpp"

#include "geometry/geometry_document.hpp"
#include "geometry/geometry_entity.hpp"
#include "mesh/mesh_document.hpp"
#include "mesh/mesh_element.hpp"
#include "mesh/mesh_node.hpp"
#include "util/logger.hpp"

#include <BRep_Builder.hxx>
#include <gmsh.h>

#include <kangaroo/util/component_factory.hpp>
#include <unordered_map>

namespace OpenGeoLab::Mesh {

#define ERROR_AND_RETURN(msg)                                                                      \
    do {                                                                                           \
        response["success"] = false;                                                               \
        response["error"] = msg;                                                                   \
        return response;                                                                           \
    } while(0)

namespace {

/**
 * @brief Map a Gmsh element type code to our MeshElementType.
 * @return MeshElementType::Invalid for unsupported types.
 */
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
    default:
        return MeshElementType::Invalid;
    }
}

} // namespace

nlohmann::json GenerateMeshAction::execute(const nlohmann::json& params,
                                           Util::ProgressCallback progress_callback) {
    nlohmann::json response;

    LOG_TRACE("GenerateMeshAction: Executing with params: {}", params.dump());
    if(!progress_callback(0.05, "Starting parse parameters...")) {
        ERROR_AND_RETURN("Operation cancelled");
    }
    if(!params.is_object()) {
        ERROR_AND_RETURN("Invalid params: expected JSON object");
    }

    const auto entities_it = params.find("entities");
    if(entities_it == params.end() || !entities_it->is_array()) {
        ERROR_AND_RETURN("Missing or invalid 'entities' array");
    }

    Geometry::EntityRefSet entities;
    for(const auto& entity_obj : *entities_it) {
        if(entity_obj.contains("uid") && entity_obj["uid"].is_number_unsigned() &&
           entity_obj.contains("type") && entity_obj["type"].is_string()) {
            const Geometry::EntityUID entity_uid = entity_obj["uid"].get<Geometry::EntityUID>();
            const Geometry::EntityType entity_type =
                Geometry::entityTypeFromString(entity_obj["type"].get<std::string>());
            if(entity_type == Geometry::EntityType::None) {
                ERROR_AND_RETURN("Invalid entity type in 'entities'");
            }
            entities.emplace(Geometry::EntityRef(entity_uid, entity_type));
        }
    }

    const double element_size = params.value("elementSize", 1.0);
    if(!(element_size > 0.0)) {
        ERROR_AND_RETURN("Invalid 'elementSize' parameter: must be positive number");
    }

    const int mesh_dimension = params.value("meshDimension", 2);
    if(mesh_dimension != 2 && mesh_dimension != 3) {
        ERROR_AND_RETURN("Invalid 'meshDimension': must be 2 or 3");
    }

    const std::string element_type = params.value("elementType", "triangle");
    if(element_type != "triangle" && element_type != "quad" && element_type != "auto") {
        ERROR_AND_RETURN("Invalid 'elementType': must be 'triangle', 'quad', or 'auto'");
    }

    if(entities.empty()) {
        ERROR_AND_RETURN("No valid entities provided for meshing");
    }

    auto shape = createShapeFromFaceEntities(entities);

    if(!progress_callback(0.2, "Starting mesh generation...")) {
        ERROR_AND_RETURN("Operation cancelled");
    }

    // Clear existing mesh data before generating new mesh
    auto mesh_doc = MeshDocumentInstance;
    mesh_doc->clear();

    // Initialize gmsh and import the compound shape
    gmsh::initialize();
    gmsh::option::setNumber("General.Terminal", 0);
    gmsh::model::add("mesh_model");

    try {
        importShapeToGmshAndMesh(shape, element_size, mesh_dimension, element_type,
                                 progress_callback);
    } catch(const std::exception& e) {
        gmsh::finalize();
        LOG_ERROR("GenerateMeshAction: Gmsh error: {}", e.what());
        ERROR_AND_RETURN(std::string("Gmsh import failed: ") + e.what());
    }

    gmsh::finalize();

    LOG_INFO("GenerateMeshAction: Generated {} nodes, {} elements", mesh_doc->nodeCount(),
             mesh_doc->elementCount());

    response["success"] = true;
    response["nodeCount"] = mesh_doc->nodeCount();
    response["elementCount"] = mesh_doc->elementCount();

    progress_callback(1.0, "Mesh generation complete");
    return response;
}

TopoDS_Shape
GenerateMeshAction::createShapeFromFaceEntities(const Geometry::EntityRefSet& entities) {

    auto doc = GeoDocumentInstance;
    std::vector<Geometry::GeometryEntityPtr> face_entity_ptrs;
    for(const auto& entity_ref : entities) {
        if(entity_ref.m_type == Geometry::EntityType::Part ||
           entity_ref.m_type == Geometry::EntityType::Solid) {
            auto part_entity = doc->findByRef(entity_ref);
            if(!part_entity) {
                LOG_WARN("GenerateMeshAction: entity not found: uid={}", entity_ref.m_uid);
                continue;
            }
            auto related_faces = doc->findRelatedEntities(entity_ref, Geometry::EntityType::Face);
            for(const auto& face_key : related_faces) {
                auto face_entity = doc->findById(face_key.m_id);
                if(face_entity) {
                    face_entity_ptrs.push_back(face_entity);
                }
            }
        } else if(entity_ref.m_type == Geometry::EntityType::Face) {
            auto entity_ptr = doc->findByRef(entity_ref);
            if(!entity_ptr) {
                LOG_WARN("GenerateMeshAction: Entity not found: uid={}, type={}", entity_ref.m_uid,
                         Geometry::entityTypeToString(entity_ref.m_type));
                continue;
            }
            face_entity_ptrs.push_back(entity_ptr);
        }
    }

    LOG_INFO("GenerateMeshAction: Generating mesh for {} entities", face_entity_ptrs.size());

    // Build a compound shape from all face shapes for gmsh import
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);
    for(const auto& face_entity : face_entity_ptrs) {
        auto shape = face_entity->shape();
        if(!shape.IsNull()) {
            builder.Add(compound, shape);
        }
    }
    return compound;
}

void GenerateMeshAction::importShapeToGmshAndMesh(const TopoDS_Shape& shape,
                                                  double element_size,
                                                  int mesh_dimension,
                                                  const std::string& element_type,
                                                  Util::ProgressCallback progress_callback) {
    // Import OCC shape into Gmsh
    std::vector<std::pair<int, int>> out_dim_tags;
    gmsh::model::occ::importShapesNativePointer(static_cast<const void*>(&shape), out_dim_tags);
    gmsh::model::occ::synchronize();

    LOG_DEBUG("GenerateMeshAction: Imported {} dim-tags into gmsh", out_dim_tags.size());

    if(!progress_callback(0.3, "Configuring mesh parameters...")) {
        return;
    }

    // Set global mesh element size
    gmsh::option::setNumber("Mesh.MeshSizeMin", element_size);
    gmsh::option::setNumber("Mesh.MeshSizeMax", element_size * 2);

    // Configure mesh algorithm based on element type
    if(element_type == "quad") {
        // Packing of Parallelograms algorithm for quad meshing
        gmsh::option::setNumber("Mesh.Algorithm", 8);
        gmsh::option::setNumber("Mesh.RecombineAll", 1);
        gmsh::option::setNumber("Mesh.RecombinationAlgorithm", 1); // Blossom
    } else if(element_type == "auto") {
        gmsh::option::setNumber("Mesh.Algorithm", 6); // Frontal-Delaunay
    } else {
        // Default: triangle
        gmsh::option::setNumber("Mesh.Algorithm", 6); // Frontal-Delaunay
        gmsh::option::setNumber("Mesh.RecombineAll", 0);
    }

    if(!progress_callback(0.4, "Running Gmsh mesh generation...")) {
        return;
    }

    // Generate mesh at the requested dimension
    gmsh::model::mesh::generate(mesh_dimension);

    if(!progress_callback(0.6, "Extracting nodes...")) {
        return;
    }

    // -------------------------------------------------------------------------
    // Extract nodes from Gmsh
    // -------------------------------------------------------------------------
    std::vector<std::size_t> node_tags;
    std::vector<double> node_coords;
    std::vector<double> node_parametric_coords;
    gmsh::model::mesh::getNodes(node_tags, node_coords, node_parametric_coords);

    auto mesh_doc = MeshDocumentInstance;

    // Map Gmsh node tags to our MeshNodeId for element connectivity
    std::unordered_map<size_t, MeshNodeId> gmsh_to_local;
    gmsh_to_local.reserve(node_tags.size());

    for(size_t i = 0; i < node_tags.size(); ++i) {
        const double x = node_coords[3 * i + 0];
        const double y = node_coords[3 * i + 1];
        const double z = node_coords[3 * i + 2];
        MeshNode node(x, y, z);
        gmsh_to_local[node_tags[i]] = node.nodeId();
        mesh_doc->addNode(node);
    }

    LOG_DEBUG("GenerateMeshAction: Extracted {} nodes", node_tags.size());

    if(!progress_callback(0.75, "Extracting elements...")) {
        return;
    }

    // -------------------------------------------------------------------------
    // Extract elements from Gmsh
    // -------------------------------------------------------------------------
    std::vector<int> element_types;
    std::vector<std::vector<size_t>> element_tags;
    std::vector<std::vector<size_t>> element_node_tags;
    gmsh::model::mesh::getElements(element_types, element_tags, element_node_tags);

    for(size_t ti = 0; ti < element_types.size(); ++ti) {
        const MeshElementType our_type = gmshTypeToMeshElementType(element_types[ti]);
        if(our_type == MeshElementType::Invalid) {
            LOG_DEBUG("GenerateMeshAction: Skipping unsupported Gmsh element type {}",
                      element_types[ti]);
            continue;
        }

        const auto& tags = element_tags[ti];
        const auto& nids = element_node_tags[ti];

        // Query Gmsh for the number of nodes per element of this type.
        // The stride through nids is num_nodes (which may exceed our first-order nodeCount
        // if Gmsh generated higher-order elements).
        std::string elem_name;
        int dim = 0;
        int order = 0;
        int num_nodes = 0;
        int num_primary = 0;
        std::vector<double> param_coords;
        gmsh::model::mesh::getElementProperties(element_types[ti], elem_name, dim, order, num_nodes,
                                                param_coords, num_primary);
        const auto stride = static_cast<size_t>(num_nodes);

        for(size_t ei = 0; ei < tags.size(); ++ei) {
            MeshElement elem(our_type);
            const uint8_t nc = elem.nodeCount();
            for(uint8_t ni = 0; ni < nc; ++ni) {
                const size_t idx = ei * stride + ni;
                if(idx >= nids.size()) {
                    break;
                }
                const auto it = gmsh_to_local.find(nids[idx]);
                if(it != gmsh_to_local.end()) {
                    elem.setNodeId(ni, it->second);
                }
            }
            mesh_doc->addElement(std::move(elem));
        }
    }

    LOG_DEBUG("GenerateMeshAction: Extracted elements, total: {}", mesh_doc->elementCount());

    progress_callback(0.9, "Mesh extraction complete");
}

} // namespace OpenGeoLab::Mesh
