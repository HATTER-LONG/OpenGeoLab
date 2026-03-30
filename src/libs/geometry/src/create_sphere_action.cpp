/**
 * @file create_sphere_action.cpp
 * @brief CreateSphereAction — creates an OCC sphere via BRepPrimAPI_MakeSphere
 */

#include <opengeolab/geometry/create_sphere_action.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <opengeolab/core/logger.hpp>

#include <BRepPrimAPI_MakeSphere.hxx>
#include <gp_Pnt.hxx>

#include <array>

namespace OpenGeoLab::Geometry {

CreateSphereAction::CreateSphereAction(ShapeStore& store) : m_store(store) {}
CreateSphereAction::~CreateSphereAction() = default;

nlohmann::json CreateSphereAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Create a sphere primitive."},
        {"params",
         {{"radius",
           {{"type", "number"}, {"required", false}, {"description", "Radius (default 1.0)"}}},
          {"center",
           {{"type", "array"},
            {"required", false},
            {"description", "[x,y,z] center point (default [0,0,0])"}}},
          {"name",
           {{"type", "string"},
            {"required", false},
            {"description", "Shape name (default Sphere)"}}},
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

nlohmann::json CreateSphereAction::execute(const nlohmann::json& param,
                                           const Core::ProgressCallback& progress) {
    const double radius = param.value("radius", 1.0);
    auto name = param.value("name", std::string{});

    std::array<double, 3> center{0.0, 0.0, 0.0};
    if(param.contains("center") && param["center"].is_array()) {
        center = param["center"].get<std::array<double, 3>>();
    }

    if(progress) {
        progress(0.0, "Creating sphere...");
    }

    BRepPrimAPI_MakeSphere maker(gp_Pnt(center[0], center[1], center[2]), radius);
    maker.Build();
    if(!maker.IsDone()) {
        return {{"ok", false}, {"summary", "Sphere creation failed"}};
    }

    if(progress) {
        progress(0.3, "Registering shape...");
    }
    auto shape_id = m_store.add(name.empty() ? "Sphere" : name, maker.Shape());
    if(name.empty()) {
        name = "Sphere_" + std::to_string(shape_id);
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
            {"action", "create_sphere"},
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
