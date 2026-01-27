/**
 * @file geometry_service.cpp
 * @brief Implementation of GeometryService for backend geometry operations
 */

#include "geometry/geometry_service.hpp"
#include "geometry/geometry_creator.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Geometry {

nlohmann::json GeometryService::processRequest(const std::string& /*module_name*/,
                                               const nlohmann::json& params,
                                               App::IProgressReporterPtr progress_reporter) {
    nlohmann::json result;
    result["success"] = false;

    if(!params.contains("action") || !params["action"].is_string()) {
        const std::string error = "Missing or invalid 'action' parameter";
        LOG_ERROR("GeometryService: {}", error);
        progress_reporter->reportError(error);
        result["error"] = error;
        return result;
    }

    const std::string action = params["action"].get<std::string>();
    LOG_DEBUG("GeometryService processing action: {}", action);
    progress_reporter->reportProgress(0.0, "Processing " + action + "...");

    try {
        // Document management actions
        if(action == "newDocument") {
            return handleNewDocument(progress_reporter);
        }

        // Creation actions
        if(action == "createPoint" || action == "createLine" || action == "createBox") {
            return handleCreateAction(action, params, progress_reporter);
        }

        // Edit actions (not yet fully implemented)
        if(action == "trim" || action == "offset") {
            return handleEditAction(action, params, progress_reporter);
        }

        // Unknown action
        const std::string error = "Unknown action: " + action;
        LOG_ERROR("GeometryService: {}", error);
        progress_reporter->reportError(error);
        result["error"] = error;
        return result;

    } catch(const std::exception& e) {
        const std::string error = "Exception: " + std::string(e.what());
        LOG_ERROR("GeometryService: {}", error);
        progress_reporter->reportError(error);
        result["error"] = error;
        return result;
    }
}

nlohmann::json GeometryService::handleNewDocument(App::IProgressReporterPtr progress_reporter) {
    nlohmann::json result;

    progress_reporter->reportProgress(0.1, "Creating new document...");

    auto& manager = GeometryDocumentManager::instance();
    auto doc = manager.newDocument();

    if(!doc) {
        const std::string error = "Failed to create new document";
        LOG_ERROR("GeometryService: {}", error);
        progress_reporter->reportError(error);
        result["success"] = false;
        result["error"] = error;
        return result;
    }

    progress_reporter->reportProgress(1.0, "Document created successfully");
    LOG_INFO("GeometryService: Created new document");

    result["success"] = true;
    result["message"] = "New document created";
    result["entityCount"] = 0;
    return result;
}

nlohmann::json GeometryService::handleCreateAction(const std::string& action,
                                                   const nlohmann::json& params,
                                                   App::IProgressReporterPtr progress_reporter) {
    nlohmann::json result;

    progress_reporter->reportProgress(0.1, "Preparing to create geometry...");

    auto& manager = GeometryDocumentManager::instance();
    auto doc = manager.currentDocument();

    if(!doc) {
        // Auto-create document if none exists
        LOG_INFO("GeometryService: No current document, creating one");
        doc = manager.newDocument();
        if(!doc) {
            const std::string error = "Failed to create document for geometry";
            LOG_ERROR("GeometryService: {}", error);
            progress_reporter->reportError(error);
            result["success"] = false;
            result["error"] = error;
            return result;
        }
    }

    progress_reporter->reportProgress(0.3, "Creating geometry...");

    auto entity = GeometryCreator::createFromJson(doc, action, params);
    if(!entity) {
        const std::string error = "Failed to create geometry for action: " + action;
        LOG_ERROR("GeometryService: {}", error);
        progress_reporter->reportError(error);
        result["success"] = false;
        result["error"] = error;
        return result;
    }

    progress_reporter->reportProgress(1.0, "Geometry created successfully");

    result["success"] = true;
    result["entityId"] = static_cast<uint64_t>(entity->entityId());
    result["entityName"] = entity->name();
    result["entityType"] = entity->typeName();
    result["documentEntityCount"] = doc->entityCount();
    return result;
}

nlohmann::json GeometryService::handleEditAction(const std::string& action,
                                                 const nlohmann::json& /*params*/,
                                                 App::IProgressReporterPtr progress_reporter) {
    nlohmann::json result;

    // Placeholder implementation for edit actions
    const std::string message = "Edit action '" + action + "' is not yet implemented";
    LOG_WARN("GeometryService: {}", message);
    progress_reporter->reportProgress(1.0, message);

    result["success"] = false;
    result["error"] = message;
    return result;
}

GeometryServiceFactory::tObjectSharedPtr GeometryServiceFactory::instance() const {
    static tObjectSharedPtr singleton_instance = std::make_shared<GeometryService>();
    return singleton_instance;
}

} // namespace OpenGeoLab::Geometry
