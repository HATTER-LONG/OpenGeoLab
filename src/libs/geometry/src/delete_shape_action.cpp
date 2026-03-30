/**
 * @file delete_shape_action.cpp
 * @brief DeleteShapeAction — removes a shape from ShapeStore
 */

#include <opengeolab/geometry/delete_shape_action.hpp>
#include <opengeolab/geometry/shape_store.hpp>

namespace OpenGeoLab::Geometry {

DeleteShapeAction::DeleteShapeAction(ShapeStore& store) : m_store(store) {}
DeleteShapeAction::~DeleteShapeAction() = default;

nlohmann::json DeleteShapeAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Delete a shape from ShapeStore."},
        {"params",
         {{"shapeId",
           {{"type", "integer"},
            {"required", true},
            {"description", "Shape identifier to delete."}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"shapeId", {{"type", "integer"}, {"description", "Deleted shape identifier."}}}}}};
}

nlohmann::json DeleteShapeAction::execute(const nlohmann::json& param,
                                          const Core::ProgressCallback& progress) {
    const auto shape_id = param.value("shapeId", static_cast<uint32_t>(0));

    if(!m_store.find(shape_id)) {
        return {{"ok", false}, {"summary", "Unknown shapeId"}};
    }

    if(progress) {
        progress(0.0, "Deleting shape...");
    }
    m_store.remove(shape_id);
    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", "delete_shape"}, {"shapeId", shape_id}};
}

} // namespace OpenGeoLab::Geometry
