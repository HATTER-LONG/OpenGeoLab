/**
 * @file list_sub_shapes_action.cpp
 * @brief ListSubShapesAction — enumerates sub-shape localIds grouped by type
 */

#include <opengeolab/geometry/list_sub_shapes_action.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <TopTools_IndexedMapOfShape.hxx>

namespace OpenGeoLab::Geometry {

ListSubShapesAction::ListSubShapesAction(ShapeStore& store) : m_store(store) {}
ListSubShapesAction::~ListSubShapesAction() = default;

nlohmann::json ListSubShapesAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description",
         "List all sub-shape 1-based localIds for a shape, grouped by entity type "
         "(faces, edges, vertices, solids, wires). Use this to discover valid localId "
         "values before calling generate_mesh or select."},
        {"params",
         {{"shapeId",
           {{"type", "integer"},
            {"required", true},
            {"description", "Top-level shape identifier (0-based, from create_box / list_shapes)."}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"shapeId", {{"type", "integer"}, {"description", "Queried shape identifier."}}},
          {"name", {{"type", "string"}, {"description", "Registered shape name."}}},
          {"subShapes",
           {{"type", "object"},
            {"description",
             "Object with arrays of 1-based localIds: "
             "{faces: [1,2,...], edges: [1,2,...], vertices: [1,2,...], "
             "solids: [1,...], wires: [1,...]}"}}}}}};
}

namespace {

nlohmann::json localIdArray(const TopTools_IndexedMapOfShape& map) {
    auto arr = nlohmann::json::array();
    const auto n = map.Extent();
    arr.get_ref<nlohmann::json::array_t&>().reserve(static_cast<std::size_t>(n));
    for(int i = 1; i <= n; ++i) {
        arr.push_back(i);
    }
    return arr;
}

} // namespace

nlohmann::json ListSubShapesAction::execute(const nlohmann::json& param,
                                            const Core::ProgressCallback& progress) {
    const auto shape_id = param.value("shapeId", static_cast<uint32_t>(0));
    const auto* entry = m_store.find(shape_id);
    if(!entry) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"summary", "Unknown shapeId"}};
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true},
            {"action", ACTION_NAME},
            {"shapeId", shape_id},
            {"name", entry->name},
            {"subShapes",
             {{"solids", localIdArray(entry->solidMap)},
              {"faces", localIdArray(entry->faceMap)},
              {"edges", localIdArray(entry->edgeMap)},
              {"vertices", localIdArray(entry->vertexMap)},
              {"wires", localIdArray(entry->wireMap)}}}};
}

} // namespace OpenGeoLab::Geometry
