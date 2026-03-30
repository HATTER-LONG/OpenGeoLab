/**
 * @file create_torus_action.cpp
 * @brief CreateTorusAction — creates an OCC torus via BRepPrimAPI_MakeTorus
 */

#include <opengeolab/geometry/create_torus_action.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <opengeolab/core/logger.hpp>

#include <BRepPrimAPI_MakeTorus.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>

#include <array>

namespace OpenGeoLab::Geometry {

CreateTorusAction::CreateTorusAction(ShapeStore& store) : m_store(store) {}
CreateTorusAction::~CreateTorusAction() = default;

nlohmann::json CreateTorusAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Create a torus primitive."},
        {"params",
         {{"majorRadius",
           {{"type", "number"},
            {"required", false},
            {"description", "Major radius (default 1.0)"}}},
          {"minorRadius",
           {{"type", "number"},
            {"required", false},
            {"description", "Minor radius (default 0.25)"}}},
          {"origin",
           {{"type", "array"},
            {"required", false},
            {"description", "[x,y,z] torus center (default [0,0,0])"}}},
          {"name",
           {{"type", "string"},
            {"required", false},
            {"description", "Shape name (default Torus)"}}},
          {"tessellate",
           {{"type", "boolean"},
            {"required", false},
            {"description", "Generate tessellation data after creation (default true)"}}},
          {"linearDeflection",
           {{"type", "number"},
            {"required", false},
            {"description", "Tessellation linear deflection when tessellate=true (default 0.1)"}}},
          {"angularDeflection",
           {{"type", "number"},
            {"required", false},
            {"description",
             "Tessellation angular deflection when tessellate=true (default 0.5)"}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"shapeId", {{"type", "integer"}, {"description", "Allocated shape identifier."}}},
          {"name", {{"type", "string"}, {"description", "Resolved shape name."}}},
          {"topology",
           {{"type", "object"}, {"description", "Topology counts for the resulting shape."}}}}}};
}

nlohmann::json CreateTorusAction::execute(const nlohmann::json& param,
                                          const Core::ProgressCallback& progress) {
    const double major_radius = param.value("majorRadius", 1.0);
    const double minor_radius = param.value("minorRadius", 0.25);
    auto name = param.value("name", std::string{});

    std::array<double, 3> origin{0.0, 0.0, 0.0};
    if(param.contains("origin") && param["origin"].is_array()) {
        origin = param["origin"].get<std::array<double, 3>>();
    }

    if(progress) {
        progress(0.0, "Creating torus...");
    }

    const gp_Ax2 axis(gp_Pnt(origin[0], origin[1], origin[2]), gp_Dir(0, 0, 1));
    BRepPrimAPI_MakeTorus maker(axis, major_radius, minor_radius);
    maker.Build();
    if(!maker.IsDone()) {
        return {{"ok", false}, {"summary", "Torus creation failed"}};
    }

    if(progress) {
        progress(0.3, "Registering shape...");
    }
    auto shape_id = m_store.add(name.empty() ? "Torus" : name, maker.Shape());
    if(name.empty()) {
        name = "Torus_" + std::to_string(shape_id);
        m_store.rename(shape_id, name);
    }

    if(param.value("tessellate", true)) {
        if(progress) {
            progress(0.5, "Tessellating...");
        }
        m_store.tessellate(shape_id, TessellationParams::fromJson(param));
    }

    if(progress) {
        progress(1.0, "Done");
    }

    const auto* entry = m_store.find(shape_id);
    return {{"ok", true},
            {"action", "create_torus"},
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
