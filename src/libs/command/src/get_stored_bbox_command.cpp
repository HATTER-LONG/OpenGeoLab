#include <opengeolab/command/get_stored_bbox_command.hpp>

#include <opengeolab/geometry/bounding_box_calculator.hpp>

namespace OpenGeoLab::Command {

GetStoredBBoxCommand::GetStoredBBoxCommand(std::shared_ptr<Geometry::PointStore> store)
    : store_(std::move(store)) {}

auto GetStoredBBoxCommand::execute(const nlohmann::json& /*payload*/) -> CommandResult {
    if (store_->empty()) {
        return {
            .ok = false,
            .summary = "No points stored yet. Use geometry.set_points first.",
            .result = {},
        };
    }

    auto points = store_->points();
    auto bounding_box = Geometry::BoundingBoxCalculator::compute(points);

    nlohmann::json result;
    result["pointCount"] = points.size();
    result["boundingBox"] = {
        {"min", {{"x", bounding_box.min.x}, {"y", bounding_box.min.y}, {"z", bounding_box.min.z}}},
        {"max", {{"x", bounding_box.max.x}, {"y", bounding_box.max.y}, {"z", bounding_box.max.z}}},
    };

    return {.ok = true, .summary = "Stored bounding box computed.", .result = result};
}

auto GetStoredBBoxCommand::actionName() const noexcept -> std::string_view {
    return "geometry.get_stored_bbox";
}

} // namespace OpenGeoLab::Command
