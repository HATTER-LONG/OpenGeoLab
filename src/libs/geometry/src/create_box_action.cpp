/**
 * @file create_box_action.cpp
 * @brief CreateBoxAction — creates an OCC box via BRepPrimAPI_MakeBox
 */

#include <opengeolab/geometry/create_box_action.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <opengeolab/core/logger.hpp>

#include <BRepPrimAPI_MakeBox.hxx>
#include <gp_Pnt.hxx>

#include <array>

namespace OpenGeoLab::Geometry {

CreateBoxAction::CreateBoxAction(ShapeStore& store) : m_store(store) {}
CreateBoxAction::~CreateBoxAction() = default;

nlohmann::json CreateBoxAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Create a box primitive and register it in ShapeStore."},
        {"params",
         {{"width",
           {{"type", "number"}, {"required", false}, {"description", "Box width (default 1.0)"}}},
          {"height",
           {{"type", "number"}, {"required", false}, {"description", "Box height (default 1.0)"}}},
          {"depth",
           {{"type", "number"}, {"required", false}, {"description", "Box depth (default 1.0)"}}},
          {"origin",
           {{"type", "array"}, {"required", false}, {"description", "[x,y,z] (default [0,0,0])"}}},
          {"name",
           {{"type", "string"}, {"required", false}, {"description", "Shape name (default Box)"}}},
          {"tessellate",
           {{"type", "boolean"},
            {"required", false},
            {"description", "Auto-tessellate (default true)"}}}}},
        {"returns",
         {{"ok", {{"type", "boolean"}, {"description", "true on success"}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name"}}},
          {"shapeId", {{"type", "integer"}, {"description", "Allocated shape id"}}},
          {"topology",
           {{"type", "object"}, {"description", "Topology counts (solids/faces/edges/..."}}}}}};
}

nlohmann::json CreateBoxAction::execute(const nlohmann::json& param,
                                        const Core::ProgressCallback& progress) {
    const double width = param.value("width", 1.0);
    const double height = param.value("height", 1.0);
    const double depth = param.value("depth", 1.0);
    auto name = param.value("name", std::string{});

    std::array<double, 3> origin{0.0, 0.0, 0.0};
    if(param.contains("origin") && param["origin"].is_array()) {
        origin = param["origin"].get<std::array<double, 3>>();
    }

    LOG_INFO("CreateBoxAction: creating box ({:.2f} x {:.2f} x {:.2f})", width, height, depth);
    if(progress) {
        progress(0.0, "Creating box...");
    }

    const gp_Pnt corner(origin[0], origin[1], origin[2]);
    BRepPrimAPI_MakeBox maker(corner, width, height, depth);
    maker.Build();
    if(!maker.IsDone()) {
        return {{"ok", false}, {"summary", "Box creation failed"}};
    }

    if(progress) {
        progress(0.3, "Registering shape...");
    }
    auto shape_id = m_store.add(name.empty() ? "Box" : name, maker.Shape());
    if(name.empty()) {
        name = "Box_" + std::to_string(shape_id);
        m_store.rename(shape_id, name);
    }

    const bool do_tessellate = param.value("tessellate", true);
    if(do_tessellate) {
        if(progress) {
            progress(0.5, "Tessellating...");
        }
        const double lin = param.value("linearDeflection", 0.05);
        const double ang = param.value("angularDeflection", 0.25);
        m_store.tessellate(shape_id, lin, ang);
    }

    if(progress) {
        progress(1.0, "Done");
    }

    const auto* entry = m_store.find(shape_id);
    return {{"ok", true},
            {"action", "create_box"},
            {"shapeId", shape_id},
            {"name", name},
            {"topology",
             {{"solids", entry->solidMap.Extent()},
              {"faces", entry->faceMap.Extent()},
              {"edges", entry->edgeMap.Extent()},
              {"vertices", entry->vertexMap.Extent()},
              {"wires", entry->wireMap.Extent()}}}};
}

} // namespace OpenGeoLab::Geometry
