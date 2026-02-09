/**
 * @file generate_mesh_action.cpp
 * @brief Implementation of GenerateMeshAction
 */

#include "generate_mesh_action.hpp"

#include "geometry/geometry_document_manager.hpp"
#include "geometry/geometry_entity.hpp"
#include "util/logger.hpp"

#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <QDir>
#include <QStandardPaths>
#include <QString>
#include <gmsh.h>

#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::Mesh {
#define ERROR_AND_RETURN(msg)                                                                      \
    do {                                                                                           \
        response["success"] = false;                                                               \
        response["error"] = msg;                                                                   \
        return response;                                                                           \
    } while(0)

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

    if(entities.empty()) {
        ERROR_AND_RETURN("No valid entities provided for meshing");
    }

    auto shape = createShapeFromFaceEntities(entities);

    if(!progress_callback(0.2, "Starting mesh generation...")) {
        ERROR_AND_RETURN("Operation cancelled");
    }
    // Initialize gmsh and import the compound shape
    gmsh::initialize();
    gmsh::option::setNumber("General.Terminal", 0);
    gmsh::model::add("mesh_model");

    try {
        importShapeToGmshAndMesh(shape, element_size);
    } catch(const std::exception& e) {
        gmsh::finalize();
        LOG_ERROR("GenerateMeshAction: Gmsh error: {}", e.what());
        ERROR_AND_RETURN(std::string("Gmsh import failed: ") + e.what());
    }

    response["success"] = true;

    return response;
}

TopoDS_Shape
GenerateMeshAction::createShapeFromFaceEntities(const Geometry::EntityRefSet& entities) {

    auto doc = GeoDocumentMgrInstance->currentDocument();

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

void GenerateMeshAction::importShapeToGmshAndMesh(const TopoDS_Shape& shape, double element_size) {
    gmsh::initialize();
    gmsh::option::setNumber("General.Terminal", 0);
    gmsh::model::add("mesh_model");

    std::vector<std::pair<int, int>> out_dim_tags;
    gmsh::model::occ::importShapesNativePointer(static_cast<const void*>(&shape), out_dim_tags);
    gmsh::model::occ::synchronize();

    LOG_DEBUG("GenerateMeshAction: Imported {} dim-tags into gmsh", out_dim_tags.size());

    // Set global mesh element size
    gmsh::option::setNumber("Mesh.MeshSizeMin", element_size);
    gmsh::option::setNumber("Mesh.MeshSizeMax", element_size * 2);
    gmsh::option::setNumber("Mesh.Algorithm", 6); // Frontal-Delaunay

    // Generate 2D mesh
    gmsh::model::mesh::generate(2);

    // Get all nodes
    std::vector<std::size_t> node_tags;
    std::vector<double> node_coords;
    std::vector<double> node_parametric_coords;
    gmsh::model::mesh::getNodes(node_tags, node_coords, node_parametric_coords);

    for(size_t i = 0; i < node_tags.size(); ++i) {
    }
}

} // namespace OpenGeoLab::Mesh
