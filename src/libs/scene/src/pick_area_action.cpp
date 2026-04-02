/**
 * @file pick_area_action.cpp
 * @brief PickAreaAction implementation
 */

#include <opengeolab/scene/pick_area_action.hpp>

#include <opengeolab/scene/viewport_state.hpp>

#include <string>

namespace OpenGeoLab::Scene {

PickAreaAction::PickAreaAction(ViewportState& state) : m_state(state) {}
PickAreaAction::~PickAreaAction() = default;

nlohmann::json PickAreaAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description",
         "Queue an async box-select pick area. Results appear in SelectionState after "
         "the next render frame. Query with scene.query_selection."},
        {"params",
         {{"x0", {{"type", "number"}, {"required", true}, {"description", "Left/start X"}}},
          {"y0", {{"type", "number"}, {"required", true}, {"description", "Top/start Y"}}},
          {"x1", {{"type", "number"}, {"required", true}, {"description", "Right/end X"}}},
          {"y1", {{"type", "number"}, {"required", true}, {"description", "Bottom/end Y"}}},
          {"coordType",
           {{"type", "string"},
            {"required", false},
            {"description", "'normalized' (0-1, default) or 'pixel' (item space)"}}},
          {"pickAction",
           {{"type", "string"},
            {"required", false},
            {"description", "'Add' (default) or 'Remove'"}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"async", {{"type", "boolean"}, {"description", "Always true — results are async."}}}}}};
}

nlohmann::json PickAreaAction::execute(const nlohmann::json& param,
                                       const Core::ProgressCallback& progress) {
    PendingPickArea request;
    request.x0 = param.value("x0", 0.0F);
    request.y0 = param.value("y0", 0.0F);
    request.x1 = param.value("x1", 0.0F);
    request.y1 = param.value("y1", 0.0F);

    const auto coord_type = param.value("coordType", std::string{"normalized"});
    if(coord_type == "pixel") {
        request.coordType = PickAreaCoordType::Pixel;
    } else {
        request.coordType = PickAreaCoordType::Normalized;
    }

    const auto pick_action = param.value("pickAction", std::string{"Add"});
    if(pick_action == "Remove") {
        request.action = Core::PickAction::Remove;
    } else {
        request.action = Core::PickAction::Add;
    }

    m_state.requestPickArea(request);

    if(progress) {
        progress(1.0, "Done");
    }
    return {{"ok", true}, {"action", ACTION_NAME}, {"async", true}};
}

} // namespace OpenGeoLab::Scene
