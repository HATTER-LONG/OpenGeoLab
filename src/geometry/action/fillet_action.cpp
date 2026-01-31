/**
 * @file fillet_action.cpp
 * @brief Implementation of fillet geometry operation
 */

#include "fillet_action.hpp"
#include "../geometry_documentImpl.hpp"
#include "../geometry_document_managerImpl.hpp"
#include "util/logger.hpp"


#include <BRepFilletAPI_MakeFillet.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Solid.hxx>

namespace OpenGeoLab::Geometry {

[[nodiscard]] bool FilletAction::execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) {
    if(!params.contains("targetEntity")) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Missing 'targetEntity' parameter.");
        }
        LOG_ERROR("FilletAction: Missing 'targetEntity' parameter");
        return false;
    }

    if(!params.contains("edges") || !params["edges"].is_array()) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Missing or invalid 'edges' parameter.");
        }
        LOG_ERROR("FilletAction: Missing or invalid 'edges' parameter");
        return false;
    }

    double radius = params.value("radius", 1.0);
    if(radius <= 0.0) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Fillet radius must be positive.");
        }
        LOG_ERROR("FilletAction: Fillet radius must be positive");
        return false;
    }

    EntityId targetId = params["targetEntity"].get<EntityId>();
    std::vector<EntityId> edgeIds;
    for(const auto& edgeIdJson : params["edges"]) {
        edgeIds.push_back(edgeIdJson.get<EntityId>());
    }

    if(edgeIds.empty()) {
        if(progress_callback) {
            progress_callback(1.0, "Error: No edges specified for fillet.");
        }
        LOG_ERROR("FilletAction: No edges specified for fillet");
        return false;
    }

    if(progress_callback && !progress_callback(0.1, "Preparing fillet operation...")) {
        return false;
    }

    auto doc = std::dynamic_pointer_cast<GeometryDocumentImpl>(
        GeometryDocumentManager::instance().currentDocument());

    if(!doc) {
        if(progress_callback) {
            progress_callback(1.0, "Error: No active document.");
        }
        LOG_ERROR("FilletAction: No active document");
        return false;
    }

    auto targetEntity = doc->findById(targetId);
    if(!targetEntity) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Target entity not found.");
        }
        LOG_ERROR("FilletAction: Target entity {} not found", targetId);
        return false;
    }

    // Get the solid shape
    TopoDS_Shape targetShape = targetEntity->shape();
    if(targetShape.IsNull() || targetShape.ShapeType() != TopAbs_SOLID) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Target must be a solid.");
        }
        LOG_ERROR("FilletAction: Target entity is not a solid");
        return false;
    }

    if(progress_callback && !progress_callback(0.3, "Creating fillet...")) {
        return false;
    }

    try {
        BRepFilletAPI_MakeFillet filletMaker(targetShape);

        // Add each edge to the fillet operation
        for(EntityId edgeId : edgeIds) {
            auto edgeEntity = doc->findById(edgeId);
            if(!edgeEntity) {
                LOG_WARN("FilletAction: Edge entity {} not found, skipping", edgeId);
                continue;
            }

            TopoDS_Shape edgeShape = edgeEntity->shape();
            if(edgeShape.IsNull() || edgeShape.ShapeType() != TopAbs_EDGE) {
                LOG_WARN("FilletAction: Entity {} is not an edge, skipping", edgeId);
                continue;
            }

            filletMaker.Add(radius, TopoDS::Edge(edgeShape));
        }

        if(progress_callback && !progress_callback(0.6, "Building fillet...")) {
            return false;
        }

        filletMaker.Build();
        if(!filletMaker.IsDone()) {
            if(progress_callback) {
                progress_callback(1.0, "Error: Fillet operation failed.");
            }
            LOG_ERROR("FilletAction: Fillet operation failed");
            return false;
        }

        TopoDS_Shape result = filletMaker.Shape();
        if(result.IsNull()) {
            if(progress_callback) {
                progress_callback(1.0, "Error: Fillet produced null result.");
            }
            LOG_ERROR("FilletAction: Fillet produced null result");
            return false;
        }

        if(progress_callback && !progress_callback(0.8, "Updating document...")) {
            return false;
        }

        // Load the new shape into the document
        auto loadResult = doc->loadFromShape(result, "Filleted_Part");
        if(!loadResult.m_success) {
            if(progress_callback) {
                progress_callback(1.0, "Error: " + loadResult.m_errorMessage);
            }
            LOG_ERROR("FilletAction: Failed to load result: {}", loadResult.m_errorMessage);
            return false;
        }

        // Delete the original entity
        (void)doc->deleteEntities({targetId}, true);

        if(progress_callback) {
            progress_callback(1.0, "Fillet completed successfully.");
        }

        LOG_INFO("FilletAction: Fillet completed with radius {}", radius);
        return true;

    } catch(const Standard_Failure& e) {
        std::string msg = e.GetMessageString() ? e.GetMessageString() : "Unknown OCC error";
        if(progress_callback) {
            progress_callback(1.0, "Error: " + msg);
        }
        LOG_ERROR("FilletAction: OCC error: {}", msg);
        return false;
    }
}

} // namespace OpenGeoLab::Geometry
