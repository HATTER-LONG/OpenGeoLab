/**
 * @file rename_shape_action.cpp
 * @brief RenameShapeAction — renames a shape in ShapeStore
 */

#include <opengeolab/geometry/rename_shape_action.hpp>
#include <opengeolab/geometry/shape_store.hpp>

namespace OpenGeoLab::Geometry {

RenameShapeAction::RenameShapeAction(ShapeStore& store) : m_store(store) {}
RenameShapeAction::~RenameShapeAction() = default;

nlohmann::json RenameShapeAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Rename a shape in ShapeStore."},
        {"params",
         {{"shapeId",
           {{"type", "integer"},
            {"required", true},
            {"description", "Shape identifier to rename."}}},
          {"newName",
           {{"type", "string"},
            {"required", true},
            {"description", "New display name for the shape."}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"shapeId", {{"type", "integer"}, {"description", "Renamed shape identifier."}}},
          {"newName", {{"type", "string"}, {"description", "New name of the shape."}}}}}};
}

nlohmann::json RenameShapeAction::execute(const nlohmann::json& param,
                                          const Core::ProgressCallback& /*progress*/) {
    const auto shape_id = param.value("shapeId", static_cast<uint32_t>(0));
    const auto new_name = param.value("newName", std::string{});

    if (new_name.empty()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"summary", "newName must not be empty"}};
    }

    if (!m_store.find(shape_id)) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"summary", "Unknown shapeId"}};
    }

    m_store.rename(shape_id, new_name);

    return {{"ok", true}, {"action", ACTION_NAME}, {"shapeId", shape_id}, {"newName", new_name}};
}

} // namespace OpenGeoLab::Geometry
