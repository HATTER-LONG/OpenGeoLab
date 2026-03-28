/**
 * @file create_cylinder_action.cpp
 * @brief CreateCylinderAction — creates an OCC cylinder via BRepPrimAPI_MakeCylinder
 */

#include <opengeolab/geometry/create_cylinder_action.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <opengeolab/core/logger.hpp>

#include <BRepPrimAPI_MakeCylinder.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>

#include <array>

namespace OpenGeoLab::Geometry {

CreateCylinderAction::CreateCylinderAction(ShapeStore& store) : m_store(store) {}
CreateCylinderAction::~CreateCylinderAction() = default;

nlohmann::json CreateCylinderAction::describe() const {
    return {{"name", ACTION_NAME},
            {"description", "Create a cylinder primitive."},
            {"params",
             {{"radius",
               {{"type", "number"}, {"required", false}, {"description", "Radius (default 0.5)"}}},
              {"height",
               {{"type", "number"}, {"required", false}, {"description", "Height (default 1.0)"}}},
              {"origin", {{"type", "array"}, {"required", false}, {"description", "[x,y,z]"}}},
              {"name", {{"type", "string"}, {"required", false}, {"description", "Shape name"}}},
              {"tessellate", {{"type", "boolean"}, {"required", false}}}}}};
}

nlohmann::json CreateCylinderAction::execute(const nlohmann::json& param,
                                             const Core::ProgressCallback& progress) {
    const double radius = param.value("radius", 0.5);
    const double height = param.value("height", 1.0);
    auto name = param.value("name", std::string{});

    std::array<double, 3> origin{0.0, 0.0, 0.0};
    if(param.contains("origin") && param["origin"].is_array()) {
        origin = param["origin"].get<std::array<double, 3>>();
    }

    if(progress) {
        progress(0.0, "Creating cylinder...");
    }

    const gp_Ax2 axis(gp_Pnt(origin[0], origin[1], origin[2]), gp_Dir(0, 0, 1));
    BRepPrimAPI_MakeCylinder maker(axis, radius, height);
    maker.Build();
    if(!maker.IsDone()) {
        return {{"ok", false}, {"summary", "Cylinder creation failed"}};
    }

    if(progress) {
        progress(0.3, "Registering shape...");
    }
    auto shape_id = m_store.add(name.empty() ? "Cylinder" : name, maker.Shape());
    if(name.empty()) {
        name = "Cylinder_" + std::to_string(shape_id);
        m_store.rename(shape_id, name);
    }

    if(param.value("tessellate", true)) {
        if(progress) {
            progress(0.5, "Tessellating...");
        }
        m_store.tessellate(shape_id, param.value("linearDeflection", 0.1),
                           param.value("angularDeflection", 0.5));
    }

    if(progress) {
        progress(1.0, "Done");
    }

    const auto* entry = m_store.find(shape_id);
    return {{"ok", true},
            {"action", "create_cylinder"},
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
