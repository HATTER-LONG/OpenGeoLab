/**
 * @file import_step_action.cpp
 * @brief ImportStepAction — reads a STEP file via STEPControl_Reader
 */

#include <opengeolab/geometry/import_step_action.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <opengeolab/core/logger.hpp>

#include <STEPControl_Reader.hxx>

#include <filesystem>

namespace OpenGeoLab::Geometry {

ImportStepAction::ImportStepAction(ShapeStore& store) : m_store(store) {}
ImportStepAction::~ImportStepAction() = default;

nlohmann::json ImportStepAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Import a STEP file into ShapeStore."},
        {"params",
         {{"path",
           {{"type", "string"},
            {"required", true},
            {"description", "File system path to the STEP file."}}},
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

nlohmann::json ImportStepAction::execute(const nlohmann::json& param,
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
        progress(0.0, "Reading STEP file...");
    }

    STEPControl_Reader reader;
    auto status = reader.ReadFile(path.c_str());
    if(status != IFSelect_RetDone) {
        return {{"ok", false}, {"summary", "Failed to read STEP file: " + path}};
    }

    if(progress) {
        progress(0.3, "Transferring roots...");
    }
    reader.TransferRoots();
    const TopoDS_Shape shape = reader.OneShape();

    if(shape.IsNull()) {
        return {{"ok", false}, {"summary", "STEP file produced null shape"}};
    }

    if(progress) {
        progress(0.5, "Registering shape...");
    }
    auto shape_id = m_store.add(name.empty() ? "STEP" : name, shape);
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
            {"action", "import_step"},
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
