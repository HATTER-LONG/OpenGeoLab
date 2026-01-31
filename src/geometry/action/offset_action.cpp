/**
 * @file offset_action.cpp
 * @brief Implementation of offset geometry operation
 */

#include "offset_action.hpp"
#include "../geometry_documentImpl.hpp"
#include "../geometry_document_managerImpl.hpp"
#include "util/logger.hpp"


#include <BRepOffsetAPI_MakeOffsetShape.hxx>
#include <TopoDS_Shape.hxx>

namespace OpenGeoLab::Geometry {

[[nodiscard]] bool OffsetAction::execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) {
    if(!params.contains("sourceEntity")) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Missing 'sourceEntity' parameter.");
        }
        LOG_ERROR("OffsetAction: Missing 'sourceEntity' parameter");
        return false;
    }

    EntityId sourceId = params["sourceEntity"].get<EntityId>();
    double distance = params.value("distance", 1.0);
    bool keepOriginal = params.value("keepOriginal", true);

    if(distance == 0.0) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Offset distance cannot be zero.");
        }
        LOG_ERROR("OffsetAction: Offset distance cannot be zero");
        return false;
    }

    if(progress_callback && !progress_callback(0.1, "Preparing offset operation...")) {
        return false;
    }

    auto doc = std::dynamic_pointer_cast<GeometryDocumentImpl>(
        GeometryDocumentManager::instance().currentDocument());

    if(!doc) {
        if(progress_callback) {
            progress_callback(1.0, "Error: No active document.");
        }
        LOG_ERROR("OffsetAction: No active document");
        return false;
    }

    auto sourceEntity = doc->findById(sourceId);
    if(!sourceEntity) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Source entity not found.");
        }
        LOG_ERROR("OffsetAction: Source entity {} not found", sourceId);
        return false;
    }

    TopoDS_Shape sourceShape = sourceEntity->shape();
    if(sourceShape.IsNull()) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Source shape is null.");
        }
        LOG_ERROR("OffsetAction: Source shape is null");
        return false;
    }

    if(progress_callback && !progress_callback(0.3, "Creating offset...")) {
        return false;
    }

    try {
        // Create offset shape
        // Parameters: shape, offset, tolerance, mode, intersection, selfInter, join
        BRepOffsetAPI_MakeOffsetShape offsetMaker;
        offsetMaker.PerformByJoin(sourceShape,     // shape
                                  distance,        // offset distance
                                  1e-6,            // tolerance
                                  BRepOffset_Skin, // mode
                                  Standard_False,  // intersection
                                  Standard_False,  // selfInter
                                  GeomAbs_Arc);    // join type

        if(progress_callback && !progress_callback(0.6, "Building offset...")) {
            return false;
        }

        offsetMaker.Build();
        if(!offsetMaker.IsDone()) {
            if(progress_callback) {
                progress_callback(1.0, "Error: Offset operation failed.");
            }
            LOG_ERROR("OffsetAction: Offset operation failed");
            return false;
        }

        TopoDS_Shape result = offsetMaker.Shape();
        if(result.IsNull()) {
            if(progress_callback) {
                progress_callback(1.0, "Error: Offset produced null result.");
            }
            LOG_ERROR("OffsetAction: Offset produced null result");
            return false;
        }

        if(progress_callback && !progress_callback(0.8, "Updating document...")) {
            return false;
        }

        // Load the new shape into the document
        auto loadResult = doc->loadFromShape(result, "Offset_Part");
        if(!loadResult.m_success) {
            if(progress_callback) {
                progress_callback(1.0, "Error: " + loadResult.m_errorMessage);
            }
            LOG_ERROR("OffsetAction: Failed to load result: {}", loadResult.m_errorMessage);
            return false;
        }

        // Delete the original entity if not keeping
        if(!keepOriginal) {
            (void)doc->deleteEntities({sourceId}, true);
        }

        if(progress_callback) {
            progress_callback(1.0, "Offset completed successfully.");
        }

        LOG_INFO("OffsetAction: Offset completed with distance {}", distance);
        return true;

    } catch(const Standard_Failure& e) {
        std::string msg = e.GetMessageString() ? e.GetMessageString() : "Unknown OCC error";
        if(progress_callback) {
            progress_callback(1.0, "Error: " + msg);
        }
        LOG_ERROR("OffsetAction: OCC error: {}", msg);
        return false;
    }
}

} // namespace OpenGeoLab::Geometry
