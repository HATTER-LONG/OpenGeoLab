/**
 * @file chamfer_action.cpp
 * @brief Implementation of chamfer geometry operation
 */

#include "chamfer_action.hpp"
#include "../geometry_documentImpl.hpp"
#include "../geometry_document_managerImpl.hpp"
#include "util/logger.hpp"


#include <BRepFilletAPI_MakeChamfer.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Solid.hxx>

namespace OpenGeoLab::Geometry {

[[nodiscard]] bool ChamferAction::execute(const nlohmann::json& params,
                                          Util::ProgressCallback progress_callback) {
    if(!params.contains("targetEntity")) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Missing 'targetEntity' parameter.");
        }
        LOG_ERROR("ChamferAction: Missing 'targetEntity' parameter");
        return false;
    }

    if(!params.contains("edges") || !params["edges"].is_array()) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Missing or invalid 'edges' parameter.");
        }
        LOG_ERROR("ChamferAction: Missing or invalid 'edges' parameter");
        return false;
    }

    double distance = params.value("distance", 1.0);
    if(distance <= 0.0) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Chamfer distance must be positive.");
        }
        LOG_ERROR("ChamferAction: Chamfer distance must be positive");
        return false;
    }

    EntityId targetId = params["targetEntity"].get<EntityId>();
    std::vector<EntityId> edgeIds;
    for(const auto& edgeIdJson : params["edges"]) {
        edgeIds.push_back(edgeIdJson.get<EntityId>());
    }

    if(edgeIds.empty()) {
        if(progress_callback) {
            progress_callback(1.0, "Error: No edges specified for chamfer.");
        }
        LOG_ERROR("ChamferAction: No edges specified for chamfer");
        return false;
    }

    if(progress_callback && !progress_callback(0.1, "Preparing chamfer operation...")) {
        return false;
    }

    auto doc = std::dynamic_pointer_cast<GeometryDocumentImpl>(
        GeometryDocumentManager::instance().currentDocument());

    if(!doc) {
        if(progress_callback) {
            progress_callback(1.0, "Error: No active document.");
        }
        LOG_ERROR("ChamferAction: No active document");
        return false;
    }

    auto targetEntity = doc->findById(targetId);
    if(!targetEntity) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Target entity not found.");
        }
        LOG_ERROR("ChamferAction: Target entity {} not found", targetId);
        return false;
    }

    // Get the solid shape
    TopoDS_Shape targetShape = targetEntity->shape();
    if(targetShape.IsNull() || targetShape.ShapeType() != TopAbs_SOLID) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Target must be a solid.");
        }
        LOG_ERROR("ChamferAction: Target entity is not a solid");
        return false;
    }

    if(progress_callback && !progress_callback(0.3, "Creating chamfer...")) {
        return false;
    }

    try {
        BRepFilletAPI_MakeChamfer chamferMaker(targetShape);

        // Build edge-to-face map for chamfer operation
        TopTools_IndexedDataMapOfShapeListOfShape edgeFaceMap;
        TopExp::MapShapesAndAncestors(targetShape, TopAbs_EDGE, TopAbs_FACE, edgeFaceMap);

        // Add each edge to the chamfer operation
        for(EntityId edgeId : edgeIds) {
            auto edgeEntity = doc->findById(edgeId);
            if(!edgeEntity) {
                LOG_WARN("ChamferAction: Edge entity {} not found, skipping", edgeId);
                continue;
            }

            TopoDS_Shape edgeShape = edgeEntity->shape();
            if(edgeShape.IsNull() || edgeShape.ShapeType() != TopAbs_EDGE) {
                LOG_WARN("ChamferAction: Entity {} is not an edge, skipping", edgeId);
                continue;
            }

            TopoDS_Edge edge = TopoDS::Edge(edgeShape);

            // Find adjacent face for chamfer
            int edgeIndex = edgeFaceMap.FindIndex(edge);
            if(edgeIndex > 0) {
                const TopTools_ListOfShape& faces = edgeFaceMap(edgeIndex);
                if(!faces.IsEmpty()) {
                    TopoDS_Face face = TopoDS::Face(faces.First());
                    // Use symmetric chamfer (same distance on both sides)
                    chamferMaker.Add(distance, distance, edge, face);
                }
            }
        }

        if(progress_callback && !progress_callback(0.6, "Building chamfer...")) {
            return false;
        }

        chamferMaker.Build();
        if(!chamferMaker.IsDone()) {
            if(progress_callback) {
                progress_callback(1.0, "Error: Chamfer operation failed.");
            }
            LOG_ERROR("ChamferAction: Chamfer operation failed");
            return false;
        }

        TopoDS_Shape result = chamferMaker.Shape();
        if(result.IsNull()) {
            if(progress_callback) {
                progress_callback(1.0, "Error: Chamfer produced null result.");
            }
            LOG_ERROR("ChamferAction: Chamfer produced null result");
            return false;
        }

        if(progress_callback && !progress_callback(0.8, "Updating document...")) {
            return false;
        }

        // Load the new shape into the document
        auto loadResult = doc->loadFromShape(result, "Chamfered_Part");
        if(!loadResult.m_success) {
            if(progress_callback) {
                progress_callback(1.0, "Error: " + loadResult.m_errorMessage);
            }
            LOG_ERROR("ChamferAction: Failed to load result: {}", loadResult.m_errorMessage);
            return false;
        }

        // Delete the original entity
        (void)doc->deleteEntities({targetId}, true);

        if(progress_callback) {
            progress_callback(1.0, "Chamfer completed successfully.");
        }

        LOG_INFO("ChamferAction: Chamfer completed with distance {}", distance);
        return true;

    } catch(const Standard_Failure& e) {
        std::string msg = e.GetMessageString() ? e.GetMessageString() : "Unknown OCC error";
        if(progress_callback) {
            progress_callback(1.0, "Error: " + msg);
        }
        LOG_ERROR("ChamferAction: OCC error: {}", msg);
        return false;
    }
}

} // namespace OpenGeoLab::Geometry
