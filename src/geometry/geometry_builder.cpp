/**
 * @file geometry_builder.cpp
 * @brief Implementation of geometry builder service.
 */
#include "geometry/geometry_builder.hpp"
#include "geometry/geometry_model.hpp"
#include "geometry/occ_converter.hpp"
#include "util/logger.hpp"

#include <BRepPrimAPI_MakeBox.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>

namespace OpenGeoLab {
namespace Geometry {

nlohmann::json GeometryBuilder::processRequest(const std::string& module_name,
                                               const nlohmann::json& params,
                                               App::ProgressReporterPtr reporter) {
    nlohmann::json result;
    result["success"] = false;
    result["show"] = true;

    LOG_INFO("GeometryBuilder: Processing request for module '{}'", module_name);

    if(reporter) {
        reporter->reportProgress(0.0, "Starting geometry creation...");
    }

    if(module_name == "AddBox" || module_name == "addBox") {
        result = createBox(params);
    } else {
        result["message"] = "Unknown geometry type: " + module_name;
        LOG_ERROR("GeometryBuilder: Unknown module '{}'", module_name);
    }

    if(reporter) {
        reporter->reportProgress(1.0, "Geometry creation completed");
    }

    return result;
}

nlohmann::json GeometryBuilder::createBox(const nlohmann::json& params) {
    nlohmann::json result;
    result["success"] = false;
    result["show"] = true;
    result["operation"] = "CreateBox";

    try {
        // Extract parameters with defaults
        std::string name = params.value("name", "Box");
        double originX = params.value("originX", 0.0);
        double originY = params.value("originY", 0.0);
        double originZ = params.value("originZ", 0.0);
        double width = params.value("width", 10.0);
        double height = params.value("height", 10.0);
        double depth = params.value("depth", 10.0);

        LOG_INFO("GeometryBuilder: Creating box '{}' at ({}, {}, {}) with size ({}, {}, {})", name,
                 originX, originY, originZ, width, height, depth);

        // Validate dimensions
        if(width <= 0 || height <= 0 || depth <= 0) {
            result["message"] = "Invalid dimensions: width, height, and depth must be positive";
            LOG_ERROR("GeometryBuilder: Invalid box dimensions");
            return result;
        }

        // Create box using OpenCASCADE
        gp_Pnt origin(originX, originY, originZ);
        BRepPrimAPI_MakeBox boxMaker(origin, width, height, depth);
        boxMaker.Build();

        if(!boxMaker.IsDone()) {
            result["message"] = "Failed to create box shape";
            LOG_ERROR("GeometryBuilder: BRepPrimAPI_MakeBox failed");
            return result;
        }

        TopoDS_Shape boxShape = boxMaker.Shape();

        // Convert to our geometry format
        OccConverter converter;
        OccConverter::TessellationParams tessParams;
        tessParams.linearDeflection = 0.1;
        tessParams.angularDeflection = 0.5;

        // Get or create model in GeometryStore
        auto& store = GeometryStore::instance();
        GeometryModelPtr model = store.getModel();

        if(!model) {
            model = std::make_shared<GeometryModel>();
        }

        // Add box to model
        if(!converter.addShapeToModel(boxShape, name, *model, tessParams)) {
            result["message"] = "Failed to convert box shape";
            LOG_ERROR("GeometryBuilder: Shape conversion failed");
            return result;
        }

        // Update store (this triggers change notification)
        store.setModel(model);

        // Build result
        result["success"] = true;
        result["message"] = "Box created successfully";
        result["name"] = name;
        result["geometry"] = {{"parts", model->partCount()},
                              {"solids", model->solidCount()},
                              {"faces", model->faceCount()},
                              {"edges", model->edgeCount()},
                              {"vertices", model->vertexCount()}};

        LOG_INFO("GeometryBuilder: Box '{}' created successfully - {}", name, model->getSummary());

    } catch(const Standard_Failure& e) {
        result["message"] = std::string("OCC error: ") + e.GetMessageString();
        LOG_ERROR("GeometryBuilder: OCC exception: {}", e.GetMessageString());
    } catch(const std::exception& e) {
        result["message"] = std::string("Error: ") + e.what();
        LOG_ERROR("GeometryBuilder: Exception: {}", e.what());
    }

    return result;
}

// --- Factory Implementation ---

GeometryBuilderFactory::tObjectSharedPtr GeometryBuilderFactory::instance() const {
    static auto instance = std::make_shared<GeometryBuilder>();
    return instance;
}

} // namespace Geometry
} // namespace OpenGeoLab
