/**
 * @file import_brep_action.cpp
 * @brief ImportBrepAction — reads a BRep file via BRepTools::Read
 */

#include <opengeolab/geometry/import_brep_action.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <opengeolab/core/logger.hpp>

#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Shape.hxx>

#include <filesystem>

namespace OpenGeoLab::Geometry {

ImportBrepAction::ImportBrepAction(ShapeStore& store) : m_store(store) {}
ImportBrepAction::~ImportBrepAction() = default;

nlohmann::json ImportBrepAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Import a BRep file into ShapeStore."},
        {"params",
         {{"path",
           {{"type", "string"},
            {"required", true},
            {"description", "File system path to the BRep file."}}},
          {"name",
           {{"type", "string"},
            {"required", false},
            {"description", "Optional override for the registered shape name."}}},
          {"tessellate",
           {{"type", "boolean"},
            {"required", false},
            {"description", "Generate tessellation data after import (default true)"}}},
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

nlohmann::json ImportBrepAction::execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) {
    const auto path = param.value("path", std::string{});
    if(path.empty()) {
        return {{"ok", false}, {"summary", "Missing required 'path' parameter"}};
    }

    if(!std::filesystem::exists(path)) {
        return {{"ok", false}, {"summary", "File not found: " + path}};
    }

    auto name = param.value("name", std::string{});
    if(name.empty()) {
        name = std::filesystem::path(path).stem().string();
    }

    if(progress) {
        progress(0.0, "Reading BRep file...");
    }

    const BRep_Builder builder;
    TopoDS_Shape shape;
    if(!BRepTools::Read(shape, path.c_str(), builder)) {
        return {{"ok", false}, {"summary", "Failed to read BRep file: " + path}};
    }

    if(shape.IsNull()) {
        return {{"ok", false}, {"summary", "BRep file produced null shape"}};
    }

    if(progress) {
        progress(0.5, "Registering shape...");
    }
    auto shape_id = m_store.add(name.empty() ? "BRep" : name, shape);
    if(name.empty()) {
        name = std::filesystem::path(path).stem().string() + "_" + std::to_string(shape_id);
        m_store.rename(shape_id, name);
    }

    if(param.value("tessellate", true)) {
        if(progress) {
            progress(0.7, "Tessellating...");
        }
        m_store.tessellate(shape_id, TessellationParams::fromJson(param));
    }

    if(progress) {
        progress(1.0, "Done");
    }

    const auto* entry = m_store.find(shape_id);
    return {{"ok", true},
            {"action", "import_brep"},
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
