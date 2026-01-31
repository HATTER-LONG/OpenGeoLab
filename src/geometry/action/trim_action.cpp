/**
 * @file trim_action.cpp
 * @brief Implementation of trim geometry operation
 */

#include "trim_action.hpp"
#include "../geometry_documentImpl.hpp"
#include "../geometry_document_managerImpl.hpp"
#include "util/logger.hpp"


#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <TopoDS_Shape.hxx>


namespace OpenGeoLab::Geometry {

[[nodiscard]] bool TrimAction::execute(const nlohmann::json& params,
                                       Util::ProgressCallback progress_callback) {
    if(!params.contains("targetEntity")) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Missing 'targetEntity' parameter.");
        }
        LOG_ERROR("TrimAction: Missing 'targetEntity' parameter");
        return false;
    }

    if(!params.contains("toolEntity")) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Missing 'toolEntity' parameter.");
        }
        LOG_ERROR("TrimAction: Missing 'toolEntity' parameter");
        return false;
    }

    EntityId targetId = params["targetEntity"].get<EntityId>();
    EntityId toolId = params["toolEntity"].get<EntityId>();
    bool keepInside = params.value("keepInside", false);
    bool keepOriginal = params.value("keepOriginal", false);

    if(progress_callback && !progress_callback(0.1, "Preparing trim operation...")) {
        return false;
    }

    auto doc = std::dynamic_pointer_cast<GeometryDocumentImpl>(
        GeometryDocumentManager::instance().currentDocument());

    if(!doc) {
        if(progress_callback) {
            progress_callback(1.0, "Error: No active document.");
        }
        LOG_ERROR("TrimAction: No active document");
        return false;
    }

    auto targetEntity = doc->findById(targetId);
    auto toolEntity = doc->findById(toolId);

    if(!targetEntity) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Target entity not found.");
        }
        LOG_ERROR("TrimAction: Target entity {} not found", targetId);
        return false;
    }

    if(!toolEntity) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Tool entity not found.");
        }
        LOG_ERROR("TrimAction: Tool entity {} not found", toolId);
        return false;
    }

    TopoDS_Shape targetShape = targetEntity->shape();
    TopoDS_Shape toolShape = toolEntity->shape();

    if(targetShape.IsNull()) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Target shape is null.");
        }
        LOG_ERROR("TrimAction: Target shape is null");
        return false;
    }

    if(toolShape.IsNull()) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Tool shape is null.");
        }
        LOG_ERROR("TrimAction: Tool shape is null");
        return false;
    }

    if(progress_callback && !progress_callback(0.3, "Performing trim...")) {
        return false;
    }

    try {
        TopoDS_Shape result;

        if(keepInside) {
            // Common operation: keep the intersection
            BRepAlgoAPI_Common commonOp(targetShape, toolShape);
            commonOp.Build();
            if(!commonOp.IsDone()) {
                if(progress_callback) {
                    progress_callback(1.0, "Error: Common operation failed.");
                }
                LOG_ERROR("TrimAction: Common operation failed");
                return false;
            }
            result = commonOp.Shape();
        } else {
            // Cut operation: remove the intersection
            BRepAlgoAPI_Cut cutOp(targetShape, toolShape);
            cutOp.Build();
            if(!cutOp.IsDone()) {
                if(progress_callback) {
                    progress_callback(1.0, "Error: Cut operation failed.");
                }
                LOG_ERROR("TrimAction: Cut operation failed");
                return false;
            }
            result = cutOp.Shape();
        }

        if(result.IsNull()) {
            if(progress_callback) {
                progress_callback(1.0, "Error: Trim produced null result.");
            }
            LOG_ERROR("TrimAction: Trim produced null result");
            return false;
        }

        if(progress_callback && !progress_callback(0.7, "Updating document...")) {
            return false;
        }

        // Load the new shape into the document
        auto loadResult = doc->loadFromShape(result, "Trimmed_Part");
        if(!loadResult.m_success) {
            if(progress_callback) {
                progress_callback(1.0, "Error: " + loadResult.m_errorMessage);
            }
            LOG_ERROR("TrimAction: Failed to load result: {}", loadResult.m_errorMessage);
            return false;
        }

        // Delete the original entity if not keeping
        if(!keepOriginal) {
            (void)doc->deleteEntities({targetId}, true);
        }

        if(progress_callback) {
            progress_callback(1.0, "Trim completed successfully.");
        }

        LOG_INFO("TrimAction: Trim completed (keepInside={}, keepOriginal={})", keepInside,
                 keepOriginal);
        return true;

    } catch(const Standard_Failure& e) {
        std::string msg = e.GetMessageString() ? e.GetMessageString() : "Unknown OCC error";
        if(progress_callback) {
            progress_callback(1.0, "Error: " + msg);
        }
        LOG_ERROR("TrimAction: OCC error: {}", msg);
        return false;
    }
}

} // namespace OpenGeoLab::Geometry
