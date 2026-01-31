/**
 * @file create_action.cpp
 * @brief Implementation of geometry creation actions (box, cylinder, etc.)
 */

#include "create_action.hpp"
#include "../geometry_document_managerImpl.hpp"
#include "util/logger.hpp"

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>

namespace OpenGeoLab::Geometry {

namespace {

/**
 * @brief Create a box shape
 * @param params JSON with dimensions and optional origin
 *        Supports two formats:
 *        1. {dx, dy, dz, x, y, z} - legacy flat format
 *        2. {dimensions: {x, y, z}, origin: {x, y, z}} - nested format from QML
 * @return Created TopoDS_Shape or null shape on failure
 */
[[nodiscard]] TopoDS_Shape createBox(const nlohmann::json& params) {
    if(!params.contains("dimensions") || !params.contains("origin")) {
        LOG_ERROR("CreateAction: Box creation params missing 'dimensions' or 'origin' fields");
        throw std::invalid_argument("Invalid box creation parameters");
    }
    const auto& dim = params["dimensions"];
    if(!dim.contains("x") || !dim.contains("y") || !dim.contains("z")) {
        LOG_ERROR("CreateAction: Box 'dimensions' field must be an object");
        throw std::invalid_argument("Invalid box creation parameters");
    }
    double dx = dim.value("x", 10.0);
    double dy = dim.value("y", 10.0);
    double dz = dim.value("z", 10.0);

    const auto& orig = params["origin"];
    if(!orig.contains("x") || !orig.contains("y") || !orig.contains("z")) {
        LOG_ERROR("CreateAction: Box 'origin' field must be an object");
        throw std::invalid_argument("Invalid box creation parameters");
    }
    double x = orig.value("x", 0.0);
    double y = orig.value("y", 0.0);
    double z = orig.value("z", 0.0);

    if(dx <= 0.0 || dy <= 0.0 || dz <= 0.0) {
        LOG_ERROR("CreateAction: Box dimensions must be positive");
        throw std::invalid_argument("Invalid box creation parameters");
    }

    LOG_DEBUG("CreateAction: Creating box at ({}, {}, {}) with size ({}, {}, {})", x, y, z, dx, dy,
              dz);

    try {
        gp_Pnt corner(x, y, z);
        BRepPrimAPI_MakeBox maker(corner, dx, dy, dz);
        maker.Build();
        if(!maker.IsDone()) {
            LOG_ERROR("CreateAction: Failed to create box");
            throw std::runtime_error("Failed to create box");
        }
        return maker.Shape();
    } catch(const Standard_Failure& e) {
        LOG_ERROR("CreateAction: OCC error creating box: {}",
                  e.GetMessageString() ? e.GetMessageString() : "Unknown");
        throw std::runtime_error("Failed to create box");
    }
}

/**
 * @brief Create a cylinder shape
 * @param params JSON with radius, height, and optional axis position
 * @return Created TopoDS_Shape or null shape on failure
 */
[[nodiscard]] TopoDS_Shape createCylinder(const nlohmann::json& params) {
    double radius = params.value("radius", 5.0);
    double height = params.value("height", 10.0);

    double x = params.value("x", 0.0);
    double y = params.value("y", 0.0);
    double z = params.value("z", 0.0);

    if(radius <= 0.0 || height <= 0.0) {
        LOG_ERROR("CreateAction: Cylinder radius and height must be positive");
        throw std::invalid_argument("Invalid cylinder creation parameters");
    }

    try {
        gp_Pnt origin(x, y, z);
        gp_Dir direction(0, 0, 1); // Z-up
        gp_Ax2 axis(origin, direction);

        BRepPrimAPI_MakeCylinder maker(axis, radius, height);
        maker.Build();
        if(!maker.IsDone()) {
            LOG_ERROR("CreateAction: Failed to create cylinder");
            throw std::runtime_error("Failed to create cylinder");
        }
        return maker.Shape();
    } catch(const Standard_Failure& e) {
        LOG_ERROR("CreateAction: OCC error creating cylinder: {}",
                  e.GetMessageString() ? e.GetMessageString() : "Unknown");
        throw std::runtime_error("Failed to create cylinder");
    }
}

/**
 * @brief Create a sphere shape
 * @param params JSON with radius and optional center position
 * @return Created TopoDS_Shape or null shape on failure
 */
[[nodiscard]] TopoDS_Shape createSphere(const nlohmann::json& params) {
    double radius = params.value("radius", 5.0);

    double x = params.value("x", 0.0);
    double y = params.value("y", 0.0);
    double z = params.value("z", 0.0);

    if(radius <= 0.0) {
        LOG_ERROR("CreateAction: Sphere radius must be positive");
        throw std::invalid_argument("Invalid sphere creation parameters");
    }

    try {
        gp_Pnt center(x, y, z);
        BRepPrimAPI_MakeSphere maker(center, radius);
        maker.Build();
        if(!maker.IsDone()) {
            LOG_ERROR("CreateAction: Failed to create sphere");
            throw std::runtime_error("Failed to create sphere");
        }
        return maker.Shape();
    } catch(const Standard_Failure& e) {
        LOG_ERROR("CreateAction: OCC error creating sphere: {}",
                  e.GetMessageString() ? e.GetMessageString() : "Unknown");
        throw std::runtime_error("Failed to create sphere");
    }
}

/**
 * @brief Create a cone shape
 * @param params JSON with radius1, radius2, height, and optional axis position
 * @return Created TopoDS_Shape or null shape on failure
 */
[[nodiscard]] TopoDS_Shape createCone(const nlohmann::json& params) {
    double radius1 = params.value("radius1", 5.0); // Base radius
    double radius2 = params.value("radius2", 2.5); // Top radius
    double height = params.value("height", 10.0);

    double x = params.value("x", 0.0);
    double y = params.value("y", 0.0);
    double z = params.value("z", 0.0);

    if(radius1 < 0.0 || radius2 < 0.0 || height <= 0.0) {
        LOG_ERROR("CreateAction: Cone parameters invalid");
        throw std::invalid_argument("Invalid cone creation parameters");
    }

    try {
        gp_Pnt origin(x, y, z);
        gp_Dir direction(0, 0, 1);
        gp_Ax2 axis(origin, direction);

        BRepPrimAPI_MakeCone maker(axis, radius1, radius2, height);
        maker.Build();
        if(!maker.IsDone()) {
            LOG_ERROR("CreateAction: Failed to create cone");
            throw std::runtime_error("Failed to create cone");
        }
        return maker.Shape();
    } catch(const Standard_Failure& e) {
        LOG_ERROR("CreateAction: OCC error creating cone: {}",
                  e.GetMessageString() ? e.GetMessageString() : "Unknown");
        throw std::runtime_error("Failed to create cone");
    }
}

} // anonymous namespace

[[nodiscard]] bool CreateAction::execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) {
    if(!params.contains("type")) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Missing 'type' parameter.");
        }
        LOG_ERROR("CreateAction: Missing 'type' parameter");
        return false;
    }

    std::string type = params["type"];
    std::string part_name = params.value("name", type);

    if(progress_callback && !progress_callback(0.1, "Creating " + type + "...")) {
        return false;
    }

    TopoDS_Shape shape;

    if(type == "box") {
        shape = createBox(params);
    } else if(type == "cylinder") {
        shape = createCylinder(params);
    } else if(type == "sphere") {
        shape = createSphere(params);
    } else if(type == "cone") {
        shape = createCone(params);
    } else {
        if(progress_callback) {
            progress_callback(1.0, "Error: Unsupported shape type '" + type + "'.");
        }
        LOG_ERROR("CreateAction: Unsupported shape type '{}'", type);
        return false;
    }

    if(shape.IsNull()) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Failed to create shape.");
        }
        return false;
    }

    if(progress_callback && !progress_callback(0.5, "Building entity hierarchy...")) {
        return false;
    }

    // Add to current document using appendShape
    // (document internally handles change notification)
    auto document = GeometryDocumentManagerImpl::instance()->currentDocument();
    if(!document) {
        if(progress_callback) {
            progress_callback(1.0, "Error: No active document.");
        }
        LOG_ERROR("CreateAction: No active document");
        return false;
    }

    auto build_progress = Util::makeScaledProgressCallback(progress_callback, 0.5, 0.95);
    auto result = document->appendShape(shape, part_name, build_progress);

    if(!result.m_success) {
        if(progress_callback) {
            progress_callback(1.0, "Error: " + result.m_errorMessage);
        }
        LOG_ERROR("CreateAction: Failed to build entity hierarchy: {}", result.m_errorMessage);
        return false;
    }

    if(progress_callback) {
        progress_callback(1.0, "Created " + type + " with " + std::to_string(result.m_entityCount) +
                                   " entities.");
    }

    LOG_INFO("CreateAction: Created {} '{}' with {} entities", type, part_name,
             result.m_entityCount);

    return true;
}

} // namespace OpenGeoLab::Geometry