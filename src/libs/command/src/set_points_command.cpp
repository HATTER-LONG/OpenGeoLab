#include <opengeolab/command/set_points_command.hpp>

#include <opengeolab/geometry/bounding_box_calculator.hpp>

namespace OpenGeoLab::Command {

SetPointsCommand::SetPointsCommand(std::shared_ptr<Geometry::PointStore> store)
    : store_(std::move(store)) {}

auto SetPointsCommand::execute(const nlohmann::json& payload) -> CommandResult {
    if (!payload.contains("points") || !payload["points"].is_array()) {
        return {.ok = false, .summary = "Missing or invalid 'points' array.", .result = {}};
    }

    const auto& json_points = payload["points"];
    if (json_points.empty()) {
        return {.ok = false, .summary = "Points array must not be empty.", .result = {}};
    }

    std::vector<Geometry::Point3D> points;
    points.reserve(json_points.size());
    for (const auto& item : json_points) {
        points.push_back({
            .x = item.value("x", 0.0),
            .y = item.value("y", 0.0),
            .z = item.value("z", 0.0),
        });
    }

    store_->setPoints(points);

    auto bounding_box = Geometry::BoundingBoxCalculator::compute(points);
    nlohmann::json result;
    result["stored"] = true;
    result["pointCount"] = points.size();
    result["boundingBox"] = {
        {"min", {{"x", bounding_box.min.x}, {"y", bounding_box.min.y}, {"z", bounding_box.min.z}}},
        {"max", {{"x", bounding_box.max.x}, {"y", bounding_box.max.y}, {"z", bounding_box.max.z}}},
    };

    return {.ok = true, .summary = "Points stored successfully.", .result = result};
}

auto SetPointsCommand::actionName() const noexcept -> std::string_view {
    return "geometry.set_points";
}

} // namespace OpenGeoLab::Command
