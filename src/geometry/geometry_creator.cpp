/**
 * @file geometry_creator.cpp
 * @brief Implementation of geometry creation component
 */

#include "geometry/geometry_creator.hpp"
#include "util/logger.hpp"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt.hxx>

namespace OpenGeoLab::Geometry {

PartEntityPtr GeometryCreator::createPoint(
    GeometryDocumentPtr doc, const std::string& name, double x, double y, double z) {
    return createPoint(doc, name, gp_Pnt(x, y, z));
}

PartEntityPtr GeometryCreator::createPoint(GeometryDocumentPtr doc,
                                           const std::string& name,
                                           const gp_Pnt& point) {
    if(!doc) {
        LOG_ERROR("Cannot create point: document is null");
        return nullptr;
    }

    try {
        BRepBuilderAPI_MakeVertex maker(point);
        if(!maker.IsDone()) {
            LOG_ERROR("Failed to create vertex at ({}, {}, {})", point.X(), point.Y(), point.Z());
            return nullptr;
        }

        TopoDS_Vertex vertex = maker.Vertex();
        auto part = std::make_shared<PartEntity>(vertex);
        part->setName(name);

        if(!doc->addEntity(part)) {
            LOG_ERROR("Failed to add point entity '{}' to document", name);
            return nullptr;
        }

        LOG_INFO("Created point '{}' at ({}, {}, {})", name, point.X(), point.Y(), point.Z());
        return part;

    } catch(const Standard_Failure& e) {
        LOG_ERROR("OCC exception creating point: {}", e.GetMessageString());
        return nullptr;
    }
}

PartEntityPtr GeometryCreator::createLine(GeometryDocumentPtr doc,
                                          const std::string& name,
                                          double start_x,
                                          double start_y,
                                          double start_z,
                                          double end_x,
                                          double end_y,
                                          double end_z) {
    if(!doc) {
        LOG_ERROR("Cannot create line: document is null");
        return nullptr;
    }

    try {
        gp_Pnt start_pt(start_x, start_y, start_z);
        gp_Pnt end_pt(end_x, end_y, end_z);

        // Check for degenerate line
        if(start_pt.Distance(end_pt) < 1e-7) {
            LOG_ERROR("Cannot create degenerate line (start == end)");
            return nullptr;
        }

        BRepBuilderAPI_MakeEdge maker(start_pt, end_pt);
        if(!maker.IsDone()) {
            LOG_ERROR("Failed to create edge from ({}, {}, {}) to ({}, {}, {})", start_x, start_y,
                      start_z, end_x, end_y, end_z);
            return nullptr;
        }

        TopoDS_Edge edge = maker.Edge();
        auto part = std::make_shared<PartEntity>(edge);
        part->setName(name);

        if(!doc->addEntity(part)) {
            LOG_ERROR("Failed to add line entity '{}' to document", name);
            return nullptr;
        }

        LOG_INFO("Created line '{}' from ({}, {}, {}) to ({}, {}, {})", name, start_x, start_y,
                 start_z, end_x, end_y, end_z);
        return part;

    } catch(const Standard_Failure& e) {
        LOG_ERROR("OCC exception creating line: {}", e.GetMessageString());
        return nullptr;
    }
}

PartEntityPtr GeometryCreator::createBox(GeometryDocumentPtr doc,
                                         const std::string& name,
                                         double origin_x,
                                         double origin_y,
                                         double origin_z,
                                         double dim_x,
                                         double dim_y,
                                         double dim_z) {
    if(!doc) {
        LOG_ERROR("Cannot create box: document is null");
        return nullptr;
    }

    // Validate dimensions
    if(dim_x <= 0 || dim_y <= 0 || dim_z <= 0) {
        LOG_ERROR("Invalid box dimensions: ({}, {}, {})", dim_x, dim_y, dim_z);
        return nullptr;
    }

    try {
        gp_Pnt origin(origin_x, origin_y, origin_z);

        // Use BRepPrimAPI_MakeBox with origin point and dimensions
        BRepPrimAPI_MakeBox maker(origin, dim_x, dim_y, dim_z);
        maker.Build();
        if(!maker.IsDone()) {
            LOG_ERROR("Failed to create box at ({}, {}, {}) with dimensions ({}, {}, {})", origin_x,
                      origin_y, origin_z, dim_x, dim_y, dim_z);
            return nullptr;
        }

        TopoDS_Shape box = maker.Shape();
        auto part = std::make_shared<PartEntity>(box);
        part->setName(name);

        if(!doc->addEntity(part)) {
            LOG_ERROR("Failed to add box entity '{}' to document", name);
            return nullptr;
        }

        LOG_INFO("Created box '{}' at origin ({}, {}, {}) with dimensions ({}, {}, {})", name,
                 origin_x, origin_y, origin_z, dim_x, dim_y, dim_z);
        return part;

    } catch(const Standard_Failure& e) {
        LOG_ERROR("OCC exception creating box: {}", e.GetMessageString());
        return nullptr;
    }
}

PartEntityPtr GeometryCreator::createFromJson(GeometryDocumentPtr doc,
                                              const std::string& action,
                                              const nlohmann::json& params) {
    if(!doc) {
        LOG_ERROR("Cannot create geometry: document is null");
        return nullptr;
    }

    try {
        std::string name = params.value("name", "Unnamed");

        if(action == "createPoint") {
            const auto& coords = params.at("coordinates");
            return createPoint(doc, name, coords.value("x", 0.0), coords.value("y", 0.0),
                               coords.value("z", 0.0));
        }

        if(action == "createLine") {
            const auto& start = params.at("start");
            const auto& end = params.at("end");
            return createLine(doc, name, start.value("x", 0.0), start.value("y", 0.0),
                              start.value("z", 0.0), end.value("x", 0.0), end.value("y", 0.0),
                              end.value("z", 0.0));
        }

        if(action == "createBox") {
            const auto& origin = params.at("origin");
            const auto& dims = params.at("dimensions");
            return createBox(doc, name, origin.value("x", 0.0), origin.value("y", 0.0),
                             origin.value("z", 0.0), dims.value("x", 10.0), dims.value("y", 10.0),
                             dims.value("z", 10.0));
        }

        LOG_ERROR("Unknown creation action: {}", action);
        return nullptr;

    } catch(const nlohmann::json::exception& e) {
        LOG_ERROR("JSON parse error in createFromJson: {}", e.what());
        return nullptr;
    }
}

} // namespace OpenGeoLab::Geometry
