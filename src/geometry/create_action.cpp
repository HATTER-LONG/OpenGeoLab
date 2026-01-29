/**
 * @file create_action.cpp
 * @brief Implementation of geometry creation action
 */

#include "create_action.hpp"
#include "geometry/geometry_document.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Geometry {

bool CreateAction::execute(const nlohmann::json& params, Util::ProgressCallback progress_callback) {
    // Validate required 'type' parameter
    if(!params.contains("type") || !params["type"].is_string()) {
        LOG_ERROR("CreateAction: Missing or invalid 'type' parameter");
        if(progress_callback) {
            progress_callback(1.0, "Error: Missing 'type' parameter.");
        }
        return false;
    }

    std::string type = params["type"].get<std::string>();

    // Get optional part name
    std::string name;
    if(params.contains("name") && params["name"].is_string()) {
        name = params["name"].get<std::string>();
    }

    // Report starting
    if(progress_callback && !progress_callback(0.1, "Creating " + type + "...")) {
        return false; // Cancelled
    }

    // Dispatch to specific creation method
    bool success = false;
    if(type == "box") {
        if(name.empty()) {
            name = "Box";
        }
        success = createBox(params, name, progress_callback);
    } else if(type == "sphere") {
        if(name.empty()) {
            name = "Sphere";
        }
        success = createSphere(params, name, progress_callback);
    } else if(type == "cylinder") {
        if(name.empty()) {
            name = "Cylinder";
        }
        success = createCylinder(params, name, progress_callback);
    } else if(type == "cone") {
        if(name.empty()) {
            name = "Cone";
        }
        success = createCone(params, name, progress_callback);
    } else {
        LOG_ERROR("CreateAction: Unsupported shape type '{}'", type);
        if(progress_callback) {
            progress_callback(1.0, "Error: Unsupported shape type '" + type + "'.");
        }
        return false;
    }

    if(success && progress_callback) {
        progress_callback(1.0, "Created " + type + " successfully.");
    }

    return success;
}

bool CreateAction::createBox(const nlohmann::json& params,
                             const std::string& name,
                             Util::ProgressCallback& progress_callback) {
    // Get dimensions with defaults
    double dx = params.value("dx", 10.0);
    double dy = params.value("dy", 10.0);
    double dz = params.value("dz", 10.0);

    // Validate dimensions
    if(dx <= 0 || dy <= 0 || dz <= 0) {
        LOG_ERROR("CreateAction: Invalid box dimensions: dx={}, dy={}, dz={}", dx, dy, dz);
        if(progress_callback) {
            progress_callback(1.0, "Error: Box dimensions must be positive.");
        }
        return false;
    }

    LOG_DEBUG("Creating box: {} x {} x {}", dx, dy, dz);

    // Get current document and create the box
    auto doc = GeometryDocumentManager::instance().currentDocument();
    auto part = doc->createBox(dx, dy, dz, name);

    if(!part) {
        LOG_ERROR("CreateAction: Failed to create box");
        if(progress_callback) {
            progress_callback(1.0, "Error: Failed to create box.");
        }
        return false;
    }

    LOG_INFO("Created box '{}' ({} x {} x {})", name, dx, dy, dz);
    return true;
}

bool CreateAction::createSphere(const nlohmann::json& params,
                                const std::string& name,
                                Util::ProgressCallback& progress_callback) {
    // Get radius with default
    double radius = params.value("radius", 5.0);

    // Validate radius
    if(radius <= 0) {
        LOG_ERROR("CreateAction: Invalid sphere radius: {}", radius);
        if(progress_callback) {
            progress_callback(1.0, "Error: Sphere radius must be positive.");
        }
        return false;
    }

    LOG_DEBUG("Creating sphere: radius={}", radius);

    // Get current document and create the sphere
    auto doc = GeometryDocumentManager::instance().currentDocument();
    auto part = doc->createSphere(radius, name);

    if(!part) {
        LOG_ERROR("CreateAction: Failed to create sphere");
        if(progress_callback) {
            progress_callback(1.0, "Error: Failed to create sphere.");
        }
        return false;
    }

    LOG_INFO("Created sphere '{}' (radius={})", name, radius);
    return true;
}

bool CreateAction::createCylinder(const nlohmann::json& params,
                                  const std::string& name,
                                  Util::ProgressCallback& progress_callback) {
    // Get dimensions with defaults
    double radius = params.value("radius", 5.0);
    double height = params.value("height", 10.0);

    // Validate dimensions
    if(radius <= 0 || height <= 0) {
        LOG_ERROR("CreateAction: Invalid cylinder dimensions: radius={}, height={}", radius,
                  height);
        if(progress_callback) {
            progress_callback(1.0, "Error: Cylinder dimensions must be positive.");
        }
        return false;
    }

    LOG_DEBUG("Creating cylinder: radius={}, height={}", radius, height);

    // Get current document and create the cylinder
    auto doc = GeometryDocumentManager::instance().currentDocument();
    auto part = doc->createCylinder(radius, height, name);

    if(!part) {
        LOG_ERROR("CreateAction: Failed to create cylinder");
        if(progress_callback) {
            progress_callback(1.0, "Error: Failed to create cylinder.");
        }
        return false;
    }

    LOG_INFO("Created cylinder '{}' (radius={}, height={})", name, radius, height);
    return true;
}

bool CreateAction::createCone(const nlohmann::json& params,
                              const std::string& name,
                              Util::ProgressCallback& progress_callback) {
    // Get dimensions with defaults
    double radius1 = params.value("radius1", 5.0);
    double radius2 = params.value("radius2", 2.0);
    double height = params.value("height", 10.0);

    // Validate dimensions
    if(radius1 < 0 || radius2 < 0 || height <= 0) {
        LOG_ERROR("CreateAction: Invalid cone dimensions: radius1={}, radius2={}, height={}",
                  radius1, radius2, height);
        if(progress_callback) {
            progress_callback(1.0, "Error: Cone dimensions must be non-negative, height positive.");
        }
        return false;
    }

    if(radius1 == 0 && radius2 == 0) {
        LOG_ERROR("CreateAction: At least one radius must be positive for cone");
        if(progress_callback) {
            progress_callback(1.0, "Error: At least one cone radius must be positive.");
        }
        return false;
    }

    LOG_DEBUG("Creating cone: radius1={}, radius2={}, height={}", radius1, radius2, height);

    // Get current document and create the cone
    auto doc = GeometryDocumentManager::instance().currentDocument();
    auto part = doc->createCone(radius1, radius2, height, name);

    if(!part) {
        LOG_ERROR("CreateAction: Failed to create cone");
        if(progress_callback) {
            progress_callback(1.0, "Error: Failed to create cone.");
        }
        return false;
    }

    LOG_INFO("Created cone '{}' (radius1={}, radius2={}, height={})", name, radius1, radius2,
             height);
    return true;
}

} // namespace OpenGeoLab::Geometry