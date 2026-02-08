/**
 * @file generate_mesh_action.cpp
 * @brief Implementation of GenerateMeshAction using Gmsh for mesh generation
 */

#include "generate_mesh_action.hpp"

#include "geometry/geometry_document_manager.hpp"
#include "geometry/geometry_entity.hpp"
#include "mesh/mesh_document_manager.hpp"
#include "mesh/mesh_types.hpp"
#include "util/logger.hpp"

#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <gmsh.h>

#include <kangaroo/util/component_factory.hpp>
#include <set>

namespace OpenGeoLab::Mesh {
#define ERROR_AND_RETURN(msg)                                                                      \
    do {                                                                                           \
        response["success"] = false;                                                               \
        response["error"] = msg;                                                                   \
        return response;                                                                           \
    } while(0)

namespace {

/// Unique ID generators for mesh entities (reset on each generate call)
static MeshElementId s_nextMeshId = 1;
static MeshElementUID s_nextNodeUid = 1;
static MeshElementUID s_nextEdgeUid = 1;
static MeshElementUID s_nextTriUid = 1;
static MeshElementUID s_nextQuadUid = 1;

MeshElementId nextMeshId() { return s_nextMeshId++; }
MeshElementUID nextNodeUid() { return s_nextNodeUid++; }
MeshElementUID nextTriUid() { return s_nextTriUid++; }
MeshElementUID nextQuadUid() { return s_nextQuadUid++; }

void resetUidGenerators() {
    s_nextMeshId = 1;
    s_nextNodeUid = 1;
    s_nextEdgeUid = 1;
    s_nextTriUid = 1;
    s_nextQuadUid = 1;
}

} // namespace

using entity_pair = std::pair<Geometry::EntityUID, Geometry::EntityType>;
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

    auto doc = GeoDocumentMgrInstance->currentDocument();

    if(entities.empty()) {
        ERROR_AND_RETURN("No valid entities provided for meshing");
    }

    // Collect face entities from selected geometry (expand Part/Solid → Faces)
    std::vector<Geometry::GeometryEntityPtr> face_entity_ptrs;
    for(const auto& entity_ref : entities) {
        if(entity_ref.m_type == Geometry::EntityType::Part ||
           entity_ref.m_type == Geometry::EntityType::Solid) {
            auto part_entity = doc->findByUIDAndType(entity_ref.m_uid, entity_ref.m_type);
            if(!part_entity) {
                LOG_WARN("GenerateMeshAction: entity not found: uid={}", entity_ref.m_uid);
                continue;
            }
            auto related_faces = doc->findRelatedEntities(entity_ref.m_uid, entity_ref.m_type,
                                                          Geometry::EntityType::Face);
            for(const auto& face_key : related_faces) {
                auto face_entity = doc->findById(face_key.m_id);
                if(face_entity) {
                    face_entity_ptrs.push_back(face_entity);
                }
            }
        } else if(entity_ref.m_type == Geometry::EntityType::Face) {
            auto entity_ptr = doc->findByUIDAndType(entity_ref.m_uid, entity_ref.m_type);
            if(!entity_ptr) {
                LOG_WARN("GenerateMeshAction: Entity not found: uid={}, type={}", entity_ref.m_uid,
                         Geometry::entityTypeToString(entity_ref.m_type));
                continue;
            }
            face_entity_ptrs.push_back(entity_ptr);
        }
    }

    if(face_entity_ptrs.empty()) {
        ERROR_AND_RETURN("No valid face entities found for meshing");
    }

    LOG_INFO("GenerateMeshAction: Generating mesh for {} face entities with element size {}",
             face_entity_ptrs.size(), element_size);

    if(!progress_callback(0.2, "Starting mesh generation...")) {
        ERROR_AND_RETURN("Operation cancelled");
    }

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

    // Initialize gmsh and import the compound shape
    gmsh::initialize();
    gmsh::option::setNumber("General.Terminal", 0);
    gmsh::model::add("mesh_model");

    try {
        // Import OCC shapes via native pointer (avoids intermediate file I/O)
        std::vector<std::pair<int, int>> out_dim_tags;
        gmsh::model::occ::importShapesNativePointer(static_cast<const void*>(&compound),
                                                    out_dim_tags);
        gmsh::model::occ::synchronize();

        LOG_DEBUG("GenerateMeshAction: Imported {} dim-tags into gmsh", out_dim_tags.size());

        if(!progress_callback(0.4, "Setting mesh parameters...")) {
            gmsh::finalize();
            ERROR_AND_RETURN("Operation cancelled");
        }

        // Set global mesh element size
        gmsh::option::setNumber("Mesh.MeshSizeMin", element_size * 0.5);
        gmsh::option::setNumber("Mesh.MeshSizeMax", element_size);
        gmsh::option::setNumber("Mesh.Algorithm", 6); // Frontal-Delaunay

        if(!progress_callback(0.5, "Generating mesh...")) {
            gmsh::finalize();
            ERROR_AND_RETURN("Operation cancelled");
        }

        // Generate 2D mesh
        gmsh::model::mesh::generate(2);

        if(!progress_callback(0.7, "Extracting mesh data...")) {
            gmsh::finalize();
            ERROR_AND_RETURN("Operation cancelled");
        }

        // Extract mesh data from gmsh and populate MeshDocument
        auto mesh_doc = MeshDocumentMgrInstance->currentDocument();
        resetUidGenerators();

        // Get all nodes
        std::vector<std::size_t> node_tags;
        std::vector<double> node_coords;
        std::vector<double> node_parametric_coords;
        gmsh::model::mesh::getNodes(node_tags, node_coords, node_parametric_coords);

        // Map gmsh node tags to our MeshNode UIDs
        std::unordered_map<std::size_t, MeshElementUID> gmsh_to_uid;

        for(size_t i = 0; i < node_tags.size(); ++i) {
            MeshNode node;
            node.m_id = nextMeshId();
            node.m_uid = nextNodeUid();
            node.m_position = Geometry::Point3D(node_coords[i * 3 + 0], node_coords[i * 3 + 1],
                                                node_coords[i * 3 + 2]);
            gmsh_to_uid[node_tags[i]] = node.m_uid;
            mesh_doc->addNode(node);
        }

        LOG_DEBUG("GenerateMeshAction: Extracted {} nodes", node_tags.size());

        // Get elements per surface entity
        // Build a mapping from gmsh surface tags to geometry entity keys
        // We iterate over the gmsh surfaces and match them to our face entities
        std::vector<std::pair<int, int>> surface_entities;
        gmsh::model::getEntities(surface_entities, 2); // dim=2 for surfaces

        size_t total_tri_count = 0;
        size_t total_quad_count = 0;

        // Assign geometry entity key — use order correspondence
        // (gmsh surfaces in order → face_entity_ptrs in order)
        for(size_t si = 0; si < surface_entities.size(); ++si) {
            const auto& [dim, tag] = surface_entities[si];

            // Determine corresponding geometry entity key
            Geometry::EntityKey geo_key;
            if(si < face_entity_ptrs.size()) {
                geo_key = face_entity_ptrs[si]->entityKey();
            }

            std::vector<int> elem_types;
            std::vector<std::vector<std::size_t>> elem_tags_vec;
            std::vector<std::vector<std::size_t>> elem_node_tags_vec;
            gmsh::model::mesh::getElements(elem_types, elem_tags_vec, elem_node_tags_vec, dim, tag);

            for(size_t ti = 0; ti < elem_types.size(); ++ti) {
                int etype = elem_types[ti];
                const auto& tags = elem_tags_vec[ti];
                const auto& node_tag_list = elem_node_tags_vec[ti];

                // Determine element type and nodes-per-element
                MeshEntityType mesh_type = MeshEntityType::None;
                size_t nodes_per_elem = 0;
                if(etype == 2) { // 3-node triangle
                    mesh_type = MeshEntityType::Triangle;
                    nodes_per_elem = 3;
                } else if(etype == 3) { // 4-node quad
                    mesh_type = MeshEntityType::Quad;
                    nodes_per_elem = 4;
                } else {
                    continue; // skip other element types
                }

                for(size_t ei = 0; ei < tags.size(); ++ei) {
                    MeshElement elem;
                    elem.m_id = nextMeshId();
                    elem.m_type = mesh_type;
                    elem.m_uid =
                        (mesh_type == MeshEntityType::Triangle) ? nextTriUid() : nextQuadUid();
                    elem.m_geometryEntity = geo_key;

                    for(size_t ni = 0; ni < nodes_per_elem; ++ni) {
                        std::size_t gmsh_node_tag = node_tag_list[ei * nodes_per_elem + ni];
                        auto uid_it = gmsh_to_uid.find(gmsh_node_tag);
                        if(uid_it != gmsh_to_uid.end()) {
                            elem.m_nodeUids.push_back(uid_it->second);
                        }
                    }

                    mesh_doc->addElement(elem);

                    if(mesh_type == MeshEntityType::Triangle) {
                        ++total_tri_count;
                    } else {
                        ++total_quad_count;
                    }
                }
            }
        }

        LOG_INFO("GenerateMeshAction: Generated {} triangles, {} quads", total_tri_count,
                 total_quad_count);

        gmsh::finalize();

        if(!progress_callback(0.95, "Mesh generation complete")) {
            ERROR_AND_RETURN("Operation cancelled");
        }

        // Build response
        response["success"] = true;
        response["node_count"] = mesh_doc->nodeCount();
        response["element_count"] = mesh_doc->elementCount();

        nlohmann::json mesh_entities_arr = nlohmann::json::array();
        for(size_t si = 0; si < face_entity_ptrs.size(); ++si) {
            const auto& face = face_entity_ptrs[si];
            auto elems = mesh_doc->findElementsByGeometry(face->entityKey());
            auto nodes = mesh_doc->findNodesByGeometry(face->entityKey());

            nlohmann::json entry;
            entry["type"] = Geometry::entityTypeToString(face->entityType());
            entry["uid"] = face->entityUID();
            entry["id"] = face->entityId();
            entry["element_count"] = elems.size();
            entry["node_count"] = nodes.size();
            mesh_entities_arr.push_back(entry);
        }
        response["mesh_entities"] = mesh_entities_arr;

    } catch(const std::exception& e) {
        gmsh::finalize();
        LOG_ERROR("GenerateMeshAction: Gmsh error: {}", e.what());
        ERROR_AND_RETURN(std::string("Mesh generation failed: ") + e.what());
    }

    return response;
}

} // namespace OpenGeoLab::Mesh
